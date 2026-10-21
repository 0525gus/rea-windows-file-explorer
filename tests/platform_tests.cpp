#include "explorer/platform.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <algorithm>
#include <filesystem>
#include <set>

using namespace explorer;

class PlatformTests final : public QObject {
    Q_OBJECT
    std::shared_ptr<IPlatform> platform_;
    QString repository_, fixture_;
    QJsonObject manifest_;

    Result<Listing> browse(const QString &path, std::stop_token stop = {}, ProgressSink progress = {}) const {
        auto parsed = platform_->parseLocation(path);
        if (const auto *error = failure(parsed))
            return *error;
        return platform_->list(std::get<Location>(parsed), stop, std::move(progress));
    }
    QString fixturePath(const QString &relative) const {
        return QDir(fixture_).filePath(relative);
    }
    static std::filesystem::path nativePath(const QString &path) {
#ifdef Q_OS_WIN
        return std::filesystem::path(path.toStdWString());
#else
        return std::filesystem::path(path.toUtf8().constData());
#endif
    }
  private slots:
    void initTestCase() {
        platform_ = makePlatform();
#ifdef EXPLORER_SOURCE_DIR
        repository_ = QString::fromUtf8(EXPLORER_SOURCE_DIR);
#else
        const auto found = QFINDTESTDATA("../test-data/fixture-v1/manifest.json");
        QVERIFY2(!found.isEmpty(), "Fixture manifest is required; missing data is a test failure.");
        repository_ = QDir(QFileInfo(found).absolutePath()).absoluteFilePath(QStringLiteral("../.."));
#endif
        fixture_ = QDir(repository_).filePath(QStringLiteral("test-data/fixture-v1"));
        QFile file(fixturePath(QStringLiteral("manifest.json")));
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
        const auto bytes = file.readAll();
        QCOMPARE(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex(),
                 QByteArray("278d49240d795cabcd15be949cdc4f235722a0c8d92236854c503efc2dd14bc1"));
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(bytes, &error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        QVERIFY(document.isObject());
        manifest_ = document.object();
        QCOMPARE(
            manifest_.value(QStringLiteral("counts")).toObject().value(QStringLiteral("bulk_files")).toInt(),
            1200);
    }

    void emptyFolderIsCompleteAndCallerInputSurvives() {
        const auto path = fixturePath(QStringLiteral("read-only/empty"));
        const auto parsed = platform_->parseLocation(path);
        QVERIFY(!failure(parsed));
        QCOMPARE(std::get<Location>(parsed).callerInput, path);
        QCOMPARE(std::get<Location>(parsed).kind, Location::Kind::Directory);
        const auto result = platform_->list(std::get<Location>(parsed));
        QVERIFY2(!failure(result), failure(result) ? qPrintable(failure(result)->message) : "");
        const auto &value = std::get<Listing>(result);
        QVERIFY(value.complete);
        QVERIFY(value.entries.isEmpty());
        QCOMPARE(value.location.key, std::get<Location>(parsed).key);
    }

    void allBulkAndUnicodeNamesAndMetadataMatchManifest_data() {
        QTest::addColumn<QString>("relative");
        QTest::newRow("all-1200-bulk-files") << QStringLiteral("read-only/bulk");
        QTest::newRow("unicode-special-long-names") << QStringLiteral("read-only/unicode");
    }
    void allBulkAndUnicodeNamesAndMetadataMatchManifest() {
        QFETCH(QString, relative);
        QVector<quint64> progress;
        const auto result =
            browse(fixturePath(relative), {}, [&](const Progress &p) { progress.push_back(p.examined); });
        QVERIFY2(!failure(result), failure(result) ? qPrintable(failure(result)->message) : "");
        const auto &value = std::get<Listing>(result);
        QVERIFY(value.complete);
        QStringList actual;
        QSet<QString> uniqueKeys;
        QHash<QString, Entry> byName;
        for (const auto &e : value.entries) {
            actual.push_back(e.name);
            QVERIFY2(!e.key.isEmpty(), "Every listed item requires its adapter-owned lookup key.");
            QVERIFY2(!uniqueKeys.contains(e.key), "Two distinct fixture names shared one entry key.");
            uniqueKeys.insert(e.key);
            byName.insert(e.name, e);
            QVERIFY(!e.directory);
        }
        QStringList expected;
        for (const auto &name : manifest_.value(QStringLiteral("expected"))
                                    .toObject()
                                    .value(QStringLiteral("listing"))
                                    .toObject()
                                    .value(relative)
                                    .toArray())
            expected.push_back(name.toString());
        actual.sort(Qt::CaseSensitive);
        expected.sort(Qt::CaseSensitive);
        QCOMPARE(actual, expected); // All names, not a count or visible preview.
        if (relative.endsWith(QStringLiteral("bulk")))
            QCOMPARE(actual.size(), 1200);
        int checked = 0;
        const auto prefix = relative + QLatin1Char('/');
        for (const auto &source : manifest_.value(QStringLiteral("files")).toArray()) {
            const auto object = source.toObject();
            const auto path = object.value(QStringLiteral("path")).toString();
            if (!path.startsWith(prefix) || path.mid(prefix.size()).contains(QLatin1Char('/')))
                continue;
            const auto name = path.mid(prefix.size());
            QVERIFY(byName.contains(name));
            const auto e = byName.value(name);
            QVERIFY2(e.sizeBytes.has_value(), qPrintable(name + QStringLiteral(": missing known size")));
            QCOMPARE(*e.sizeBytes, quint64(object.value(QStringLiteral("bytes")).toInteger()));
            QVERIFY2(e.modifiedMs.has_value(),
                     qPrintable(name + QStringLiteral(": missing known modification time")));
            QCOMPARE(*e.modifiedMs,
                     object.value(QStringLiteral("modified_epoch_seconds")).toInteger() * qint64(1000));
            QVERIFY2(!e.typeName.isEmpty(), qPrintable(name + QStringLiteral(": missing type display")));
            QVERIFY2(!e.identity.isEmpty(),
                     qPrintable(name + QStringLiteral(": fixture object identity unknown")));
            ++checked;
        }
        QCOMPARE(checked, value.entries.size());
        QVERIFY2(!progress.isEmpty(), "Complete enumeration must expose progress; an empty progress trace "
                                      "does not prove responsiveness.");
        for (qsizetype i = 1; i < progress.size(); ++i)
            QVERIFY(progress[i] >= progress[i - 1]);
        QVERIFY(progress.last() >= quint64(value.entries.size()));
    }

    void nonexistentAndInvalidPathsAreErrors() {
        const auto missingPath = fixturePath(QStringLiteral("read-only/errors/does-not-exist"));
        const auto missing = browse(missingPath);
        QVERIFY(failure(missing));
        QCOMPARE(failure(missing)->code, ErrorCode::NotFound);
        QVERIFY(!failure(missing)->operation.isEmpty());
        QVERIFY(!failure(missing)->message.isEmpty());
        QString withNul = fixturePath(QStringLiteral("read-only/empty"));
        withNul.append(QChar::Null);
        withNul.append(QStringLiteral("trailing"));
        const auto invalid = platform_->parseLocation(withNul);
        QVERIFY(failure(invalid));
        QCOMPARE(failure(invalid)->code, ErrorCode::InvalidPath);
        QCOMPARE(failure(invalid)->input, withNul);
    }

    void preCancelledListDoesNotReturnSuccess() {
        std::stop_source stop;
        stop.request_stop();
        const auto result = browse(fixturePath(QStringLiteral("read-only/bulk")), stop.get_token());
        QVERIFY(failure(result));
        QCOMPARE(failure(result)->code, ErrorCode::Cancelled);
    }

    void realDeniedSourceGateRemainsUnread() {
#ifdef Q_OS_WIN
        // Deliberately use only the adapter. No QFile/QDir/OS enumeration of the
        // locked source gate, and no attempt to unlock it or inspect descendants.
        const auto result = browse(QDir(repository_).filePath(QStringLiteral("verification/source")));
        QVERIFY2(failure(result), "The locked source directory must not enumerate successfully.");
        QCOMPARE(failure(result)->code, ErrorCode::PermissionDenied);
        QVERIFY(!failure(result)->message.isEmpty());
#else
        QSKIP("NTFS source-gate ACL is a Windows-only denial check; this is not Linux permission coverage.");
#endif
    }

    void adapterNaturalComparatorOrdersNumbers() {
        QVERIFY(platform_->compareNames(QStringLiteral("file2"), QStringLiteral("file10")) < 0);
        QVERIFY(platform_->compareNames(QStringLiteral("file10"), QStringLiteral("file2")) > 0);
        QCOMPARE(platform_->compareNames(QStringLiteral("same"), QStringLiteral("same")), 0);
    }

    void staleEntryOpenFailsBeforeDelegation_data() {
        QTest::addColumn<bool>("replace");
        QTest::newRow("externally-deleted") << false;
        QTest::newRow("same-path-different-object") << true;
    }
    void staleEntryOpenFailsBeforeDelegation() {
        QFETCH(bool, replace);
        const auto digest = [](const QString &path) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly))
                return QByteArray{};
            return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex();
        };
        const QString sourceRelative = QStringLiteral("read-only/errors/normal-readable.txt");
        QByteArray sourceExpected;
        for (const auto &value : manifest_.value(QStringLiteral("files")).toArray()) {
            const auto row = value.toObject();
            if (row.value(QStringLiteral("path")).toString() == sourceRelative)
                sourceExpected = row.value(QStringLiteral("sha256")).toString().toLatin1();
        }
        QVERIFY(!sourceExpected.isEmpty());
        QCOMPARE(digest(fixturePath(sourceRelative)), sourceExpected);
        QTemporaryDir scratch(
            QDir(repository_).filePath(QStringLiteral("test-data/runtime-stale-open-XXXXXX")));
        QVERIFY(scratch.isValid());
        const auto write = [](const QString &path, const QByteArray &bytes) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly | QIODevice::NewOnly) &&
                   file.write(bytes) == bytes.size() && file.flush();
        };
        const auto target = scratch.filePath(QStringLiteral("target.rea-validation-stale"));
        const auto neighbor = scratch.filePath(QStringLiteral("neighbor.txt"));
        QVERIFY(write(target, QByteArray("original-object")));
        QVERIFY(write(neighbor, QByteArray("untouched-neighbor")));
        const auto neighborDigest = digest(neighbor);
        const auto before = browse(scratch.path());
        QVERIFY(!failure(before));
        const auto &entries = std::get<Listing>(before).entries;
        const auto found = std::find_if(entries.begin(), entries.end(), [](const Entry &e) {
            return e.name == QStringLiteral("target.rea-validation-stale");
        });
        QVERIFY(found != entries.end());
        const Entry stale = *found;
        QVERIFY(!stale.identity.isEmpty());
        if (replace) {
            // Create the replacement while the original still exists, so a
            // filesystem cannot immediately recycle the old inode/file index.
            const auto replacement = scratch.filePath(QStringLiteral("replacement.rea-validation-stale"));
            QVERIFY(write(replacement, QByteArray("different-object")));
            const auto replacementList = browse(scratch.path());
            QVERIFY(!failure(replacementList));
            const auto &candidates = std::get<Listing>(replacementList).entries;
            const auto next = std::find_if(candidates.begin(), candidates.end(), [](const Entry &e) {
                return e.name == QStringLiteral("replacement.rea-validation-stale");
            });
            QVERIFY(next != candidates.end());
            QVERIFY(!next->identity.isEmpty());
            QVERIFY(next->identity != stale.identity);
            QVERIFY(QFile::remove(target));
            QVERIFY(QFile::rename(replacement, target));
            const auto after = browse(scratch.path());
            QVERIFY(!failure(after));
            const auto &current = std::get<Listing>(after).entries;
            const auto now = std::find_if(current.begin(), current.end(), [](const Entry &e) {
                return e.name == QStringLiteral("target.rea-validation-stale");
            });
            QVERIFY(now != current.end());
            QVERIFY(now->identity != stale.identity); // Reject unknown/unchanged setup before calling open.
        } else {
            QVERIFY(QFile::remove(target));
        }
        // Correct production adapters return at the existence/identity boundary,
        // before ShellExecute/xdg-open. No success-open path is invoked here.
        const auto result = platform_->open(stale);
        QVERIFY2(failure(result), "A stale listed object must not be delegated to a default application.");
        QCOMPARE(failure(result)->code, replace ? ErrorCode::Changed : ErrorCode::NotFound);
        QVERIFY(!failure(result)->message.isEmpty());
        QCOMPARE(digest(neighbor), neighborDigest);
        QCOMPARE(digest(fixturePath(sourceRelative)), sourceExpected);
        QCOMPARE(digest(fixturePath(QStringLiteral("manifest.json"))),
                 QByteArray("278d49240d795cabcd15be949cdc4f235722a0c8d92236854c503efc2dd14bc1"));
        if (replace)
            QCOMPARE(
                digest(target),
                QCryptographicHash::hash(QByteArray("different-object"), QCryptographicHash::Sha256).toHex());
        else
            QVERIFY(!QFile::exists(target));
    }

    void directorySymlinkAndCycleAreFiniteAndIdentified() {
        QTemporaryDir fixture(
            QDir(repository_).filePath(QStringLiteral("test-data/runtime-platform-links-XXXXXX")));
        QVERIFY(fixture.isValid());
        QVERIFY(QDir().mkpath(fixture.filePath(QStringLiteral("target"))));
        std::error_code error;
        std::filesystem::create_directory_symlink(nativePath(fixture.filePath(QStringLiteral("target"))),
                                                  nativePath(fixture.filePath(QStringLiteral("link"))),
                                                  error);
        if (error)
            QSKIP(qPrintable(QStringLiteral("Directory symlink creation unavailable: ") +
                             QString::fromStdString(error.message())));
        std::filesystem::create_directory_symlink(
            nativePath(fixture.path()), nativePath(fixture.filePath(QStringLiteral("target/loop"))), error);
        QVERIFY2(!error, error ? error.message().c_str() : "");
        const auto root = browse(fixture.path());
        QVERIFY(!failure(root));
        const auto &entries = std::get<Listing>(root).entries;
        QCOMPARE(entries.size(), 2);
        const auto link = std::find_if(entries.begin(), entries.end(),
                                       [](const Entry &e) { return e.name == QStringLiteral("link"); });
        QVERIFY(link != entries.end());
        QVERIFY(link->link);
        const auto target = browse(fixture.filePath(QStringLiteral("target")));
        QVERIFY(!failure(target));
        QCOMPARE(std::get<Listing>(target).entries.size(), 1);
        QVERIFY(std::get<Listing>(target).entries.first().link);
        QVERIFY(std::get<Listing>(root).complete);
        QVERIFY(std::get<Listing>(target).complete);
        // One-level listings expose links; they do not recursively follow loop.
        // This does not substitute for future recursive search/copy loop tests.
    }
};

QTEST_GUILESS_MAIN(PlatformTests)
#include "platform_tests.moc"
