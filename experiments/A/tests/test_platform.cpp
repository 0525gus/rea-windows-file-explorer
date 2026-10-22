#include "explorer/platform.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <filesystem>

using namespace explorer;
class PlatformTest final : public QObject {
    Q_OBJECT
    const QString fixture_ = QDir::cleanPath(QString::fromUtf8(EXPLORER_FIXTURE_ROOT));
    std::shared_ptr<Platform> platform_ = makeFilesystemPlatform();
    Listing listing(const QString &relative) {
        const auto result = platform_->list(Location::folder(fixture_ + "/" + relative), cancellation());
        if (const auto *value = std::get_if<Listing>(&result))
            return *value;
        QTest::qFail(qPrintable(std::get<Error>(result).message()), __FILE__, __LINE__);
        return {};
    }
  private slots:
    void emptyFolder() {
        QCOMPARE(listing("read-only/empty").entries.size(), 0);
    }
    void bulkComplete() {
        const auto value = listing("read-only/bulk");
        QCOMPARE(value.entries.size(), 1200);
        QSet<QString> names;
        qint64 bytes = 0;
        for (const auto &item : value.entries) {
            names.insert(item.name);
            QVERIFY(!item.directory);
            QVERIFY(item.modified.isValid());
            bytes += item.size;
        }
        QCOMPARE(names.size(), 1200);
        QVERIFY(names.contains("file-0000.bin"));
        QVERIFY(names.contains("file-1199.bin"));
        QVERIFY(bytes > 900000);
    }
    void unicodeAndMetadata() {
        const auto value = listing("read-only/unicode");
        QCOMPARE(value.entries.size(), 6);
        QSet<QString> names;
        qint64 bytes = 0;
        for (const auto &item : value.entries) {
            names.insert(item.name);
            bytes += item.size;
            QVERIFY(item.modified.isValid());
            QVERIFY(item.path.endsWith(item.name));
            QVERIFY(!item.type.isEmpty());
        }
        QVERIFY(names.contains(QString::fromUtf8("한글_파일.txt")));
        QVERIFY(names.contains(QString::fromUtf8("한글_특수문자_é_😀.txt")));
        QVERIFY(names.contains(QString::fromUtf8("café.txt")));
        QCOMPARE(bytes, 376);
    }
    void invalidAndNotDirectory() {
        const auto missing = platform_->list(Location::folder(fixture_ + "/does-not-exist"), cancellation());
        QVERIFY(std::holds_alternative<Error>(missing));
        QCOMPARE(std::get<Error>(missing).code, ErrorCode::NotFound);
        const auto file =
            platform_->list(Location::folder(fixture_ + "/read-only/unicode/café.txt"), cancellation());
        QVERIFY(std::holds_alternative<Error>(file));
        QCOMPARE(std::get<Error>(file).code, ErrorCode::NotDirectory);
    }
    void deniedRealAcl() {
#ifdef Q_OS_WIN
        const auto result = platform_->list(
            Location::folder("C:/dev/Windows_fileexplore/verification/source"), cancellation());
        QVERIFY(std::holds_alternative<Error>(result));
        QCOMPARE(std::get<Error>(result).code, ErrorCode::AccessDenied);
#else
        QSKIP(
            "Windows ACL-denied source gate is not a Linux permission fixture; no ACL mutation authorized.");
#endif
    }
    void externalDeletion() {
        QTemporaryDir temp(QCoreApplication::applicationDirPath() + "/test-work-XXXXXX");
        QVERIFY(temp.isValid());
        const auto path = temp.filePath("gone.txt");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("safe test");
        file.close();
        const auto before = platform_->list(Location::folder(temp.path()), cancellation());
        QVERIFY(std::holds_alternative<Listing>(before));
        QCOMPARE(std::get<Listing>(before).entries.size(), 1);
        QVERIFY(QFile::remove(path));
        const auto opened = platform_->openFile(path);
        QVERIFY(std::holds_alternative<Error>(opened));
        QCOMPARE(std::get<Error>(opened).code, ErrorCode::NotFound);
        const auto after = platform_->list(Location::folder(temp.path()), cancellation());
        QCOMPARE(std::get<Listing>(after).entries.size(), 0);
    }
    void cancelledList() {
        const auto token = explorer::cancellation();
        token->store(true);
        const auto result = platform_->list(Location::folder(fixture_), token);
        QVERIFY(std::holds_alternative<Error>(result));
        QCOMPARE(std::get<Error>(result).code, ErrorCode::Cancelled);
    }
    void linksAreFactsNotRecursion() {
        QTemporaryDir temp(QCoreApplication::applicationDirPath() + "/test-work-XXXXXX");
        QVERIFY(temp.isValid());
        std::error_code ec;
#ifdef Q_OS_WIN
        const auto base = std::filesystem::path(temp.path().toStdWString());
#else
        const auto base = std::filesystem::path(temp.path().toStdString());
#endif
        std::filesystem::create_directory_symlink(base, base / "cycle", ec);
        if (ec)
            QSKIP(qPrintable("Symlink creation unavailable: " + QString::fromStdString(ec.message())));
        const auto result = platform_->list(Location::folder(temp.path()), cancellation());
        QVERIFY(std::holds_alternative<Listing>(result));
        const auto entries = std::get<Listing>(result).entries;
        QCOMPARE(entries.size(), 1);
        QVERIFY(entries.front().link);
        QVERIFY(entries.front().directory);
        std::filesystem::remove(base / "cycle", ec);
        QVERIFY(!ec);
    }
    void rootsAndCapabilities() {
        const auto result = platform_->list({}, cancellation());
        QVERIFY(std::holds_alternative<Listing>(result));
        QVERIFY(!std::get<Listing>(result).entries.isEmpty());
        QVERIFY(platform_->capabilities().browse);
        QVERIFY(!platform_->capabilities().nativeProperties);
    }
};
QTEST_GUILESS_MAIN(PlatformTest)
#include "test_platform.moc"
