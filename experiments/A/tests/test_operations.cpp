#include "explorer/operation_manager.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <filesystem>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <Windows.h>
#endif
using namespace explorer;
namespace {
void write(const QString &path, const QByteArray &data) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        qFatal("Cannot write owned test fixture");
    if (file.write(data) != data.size())
        qFatal("Cannot write owned test bytes");
}
QByteArray digest(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
}
OperationRequest request(OperationKind kind, const QStringList &sources, const QString &destination = {}) {
    return {kind, sources, destination, {}, CollisionPolicy::Ask, false};
}
} // namespace
class OperationsTest final : public QObject {
    Q_OBJECT
    std::shared_ptr<Platform> platform_ = makeFilesystemPlatform();
    QString ownedTemplate() const {
        return QCoreApplication::applicationDirPath() + "/mutation-work-XXXXXX";
    }
    OperationReport successful(const OperationRequest &value) {
        const auto result = platform_->mutate(value, {}, cancellation());
        if (const auto *error = std::get_if<Error>(&result)) {
            QTest::qFail(qPrintable(error->message()), __FILE__, __LINE__);
            return {};
        }
        const auto report = std::get<OperationReport>(result);
        if (!report.success()) {
            for (const auto &item : report.items)
                if (item.error)
                    qWarning().noquote() << item.error->message();
            QTest::qFail("Operation report has item failures", __FILE__, __LINE__);
        }
        return report;
    }
  private slots:
    void multiCopyUnicodeAndTree() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir base(temp.path());
        QVERIFY(base.mkdir("source"));
        QVERIFY(base.mkdir("target"));
        QVERIFY(base.mkpath("source/folder/child"));
        const auto first = temp.filePath(QString::fromUtf8("source/한글_é_😀.txt"));
        write(first, "unicode source");
        const auto nested = temp.filePath("source/folder/child/payload.bin");
        write(nested, QByteArray(300000, 'x'));
        QVector<OperationProgress> progress;
        const auto result = platform_->mutate(
            request(OperationKind::Copy, {first, temp.filePath("source/folder")}, temp.filePath("target")),
            [&](const auto &p) { progress.push_back(p); }, cancellation());
        QVERIFY(std::holds_alternative<OperationReport>(result));
        QVERIFY(std::get<OperationReport>(result).success());
        QCOMPARE(digest(first), digest(temp.filePath(QString::fromUtf8("target/한글_é_😀.txt"))));
        QCOMPARE(digest(nested), digest(temp.filePath("target/folder/child/payload.bin")));
        QVERIFY(QFileInfo::exists(first));
        QVERIFY(QFileInfo::exists(nested));
        QVERIFY(!progress.isEmpty());
        QCOMPARE(progress.back().items, 2);
        QVERIFY(progress.back().bytes >= 300000);
        QCOMPARE(base.entryList({".explorer-a-*"}, QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot).size(),
                 0);
    }
    void conflictPreflightIsAllOrNothing() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("target"));
        const auto first = temp.filePath("one"), second = temp.filePath("two");
        write(first, "new1");
        write(second, "new2");
        write(temp.filePath("target/two"), "old2");
        const auto result = platform_->mutate(
            request(OperationKind::Copy, {first, second}, temp.filePath("target")), {}, cancellation());
        QVERIFY(std::holds_alternative<Error>(result));
        QCOMPARE(std::get<Error>(result).code, ErrorCode::AlreadyExists);
        QVERIFY(!QFileInfo::exists(temp.filePath("target/one")));
        QCOMPARE(digest(second), QCryptographicHash::hash("new2", QCryptographicHash::Sha256));
        QCOMPARE(digest(temp.filePath("target/two")),
                 QCryptographicHash::hash("old2", QCryptographicHash::Sha256));
    }
    void skipAndConfirmedReplace() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("target"));
        const auto source = temp.filePath("file");
        write(source, "new");
        write(temp.filePath("target/file"), "old");
        auto value = request(OperationKind::Copy, {source}, temp.filePath("target"));
        value.collision = CollisionPolicy::Skip;
        const auto skipped = successful(value);
        QCOMPARE(skipped.items.size(), 1);
        QVERIFY(skipped.items.front().skipped);
        QVERIFY(digest(source) != digest(temp.filePath("target/file")));
        value.collision = CollisionPolicy::Replace;
        const auto denied = platform_->mutate(value, {}, cancellation());
        QVERIFY(std::holds_alternative<Error>(denied));
        QCOMPARE(std::get<Error>(denied).code, ErrorCode::AccessDenied);
        value.confirmedDestructive = true;
        successful(value);
        QCOMPARE(digest(source), digest(temp.filePath("target/file")));
        QVERIFY(QDir(temp.filePath("target"))
                    .entryList({".explorer-a-*"}, QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot)
                    .isEmpty());
    }
    void moveAndRenameAndMkdir() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("target"));
        const auto source = temp.filePath("source");
        write(source, "move bytes");
        const auto expected = digest(source);
        successful(request(OperationKind::Move, {source}, temp.filePath("target")));
        QVERIFY(!QFileInfo::exists(source));
        QCOMPARE(digest(temp.filePath("target/source")), expected);
        auto rename = request(OperationKind::Rename, {temp.filePath("target/source")});
        rename.name = QString::fromUtf8("새 이름.txt");
        successful(rename);
        QCOMPARE(digest(temp.filePath(QString::fromUtf8("target/새 이름.txt"))), expected);
        auto mkdir = request(OperationKind::NewFolder, {}, temp.filePath("target"));
        mkdir.name = "new folder";
        successful(mkdir);
        QVERIFY(QFileInfo(temp.filePath("target/new folder")).isDir());
        auto renameFolder = request(OperationKind::Rename, {temp.filePath("target/new folder")});
        renameFolder.name = "renamed folder";
        successful(renameFolder);
        QVERIFY(QFileInfo(temp.filePath("target/renamed folder")).isDir());
    }
    void moveCollisionSkipThenReplace() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("target"));
        const auto source = temp.filePath("source");
        const auto destination = temp.filePath("target/source");
        write(source, "new move bytes");
        write(destination, "old destination");
        const auto expected = digest(source);
        auto value = request(OperationKind::Move, {source}, temp.filePath("target"));
        const auto collision = platform_->mutate(value, {}, cancellation());
        QVERIFY(std::holds_alternative<Error>(collision));
        QCOMPARE(std::get<Error>(collision).code, ErrorCode::AlreadyExists);
        value.collision = CollisionPolicy::Skip;
        const auto skipped = successful(value);
        QVERIFY(skipped.items.front().skipped);
        QVERIFY(QFileInfo::exists(source));
        value.collision = CollisionPolicy::Replace;
        value.confirmedDestructive = true;
        const auto moved = successful(value);
        QVERIFY(moved.items.front().succeeded);
        QVERIFY(!QFileInfo::exists(source));
        QCOMPARE(digest(destination), expected);
    }
    void copyPreservesModifiedTime() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("target"));
        const auto source = temp.filePath("source");
        write(source, "metadata bytes");
        QFile file(source);
        QVERIFY(file.open(QIODevice::ReadWrite));
        const auto time = QDateTime::fromMSecsSinceEpoch(1577836800000LL, Qt::UTC);
        QVERIFY(file.setFileTime(time, QFileDevice::FileModificationTime));
        file.close();
        const auto before = QFileInfo(source).lastModified();
        successful(request(OperationKind::Copy, {source}, temp.filePath("target")));
        QCOMPARE(QFileInfo(temp.filePath("target/source")).lastModified(), before);
    }
    void occupiedDirectoryNeverMerged() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkpath("source/folder"));
        QVERIFY(dir.mkpath("target/folder"));
        write(temp.filePath("source/folder/new"), "new");
        write(temp.filePath("target/folder/old"), "old");
        auto value = request(OperationKind::Copy, {temp.filePath("source/folder")}, temp.filePath("target"));
        value.collision = CollisionPolicy::Replace;
        value.confirmedDestructive = true;
        const auto result = platform_->mutate(value, {}, cancellation());
        QVERIFY(std::holds_alternative<Error>(result));
        QCOMPARE(std::get<Error>(result).code, ErrorCode::Unsupported);
        QVERIFY(!QFileInfo::exists(temp.filePath("target/folder/new")));
        QVERIFY(QFileInfo::exists(temp.filePath("target/folder/old")));
    }
    void lockedSourceCopyPreservesBothSides() {
#ifdef Q_OS_WIN
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("target"));
        const auto source = temp.filePath("source");
        write(source, "source must survive");
        write(temp.filePath("target/source"), "destination must survive");
        const auto native = QDir::toNativeSeparators(source).toStdWString();
        const auto handle = CreateFileW(native.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL, nullptr);
        QVERIFY(handle != INVALID_HANDLE_VALUE);
        auto value = request(OperationKind::Copy, {source}, temp.filePath("target"));
        value.collision = CollisionPolicy::Replace;
        value.confirmedDestructive = true;
        const auto result = platform_->mutate(value, {}, cancellation());
        CloseHandle(handle);
        QVERIFY(std::holds_alternative<OperationReport>(result));
        QVERIFY(!std::get<OperationReport>(result).success());
        QCOMPARE(digest(source), QCryptographicHash::hash("source must survive", QCryptographicHash::Sha256));
        QCOMPARE(digest(temp.filePath("target/source")),
                 QCryptographicHash::hash("destination must survive", QCryptographicHash::Sha256));
#else
        QSKIP("Windows mandatory sharing lock fixture is not available on Linux; permission settings were "
              "not changed.");
#endif
    }
    void renameFailureAndSourceSafety() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        const auto a = temp.filePath("a"), b = temp.filePath("b");
        write(a, "a");
        write(b, "b");
        auto value = request(OperationKind::Rename, {a});
        value.name = "../escape";
        auto result = platform_->mutate(value, {}, cancellation());
        QVERIFY(std::holds_alternative<Error>(result));
        QCOMPARE(std::get<Error>(result).code, ErrorCode::InvalidName);
        value.name = "b";
        result = platform_->mutate(value, {}, cancellation());
        QVERIFY(std::holds_alternative<Error>(result));
        QCOMPARE(std::get<Error>(result).code, ErrorCode::AlreadyExists);
        QVERIFY(QFileInfo::exists(a));
        QVERIFY(QFileInfo::exists(b));
        QFile::remove(a);
        value.name = "c";
        result = platform_->mutate(value, {}, cancellation());
        QVERIFY(std::holds_alternative<Error>(result));
        QCOMPARE(std::get<Error>(result).code, ErrorCode::NotFound);
    }
    void deleteRequiresExplicitConfirmation() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        const auto path = temp.filePath("owned");
        write(path, "owned disposable bytes");
        auto value = request(OperationKind::Remove, {path});
        const auto result = platform_->mutate(value, {}, cancellation());
        QVERIFY(std::holds_alternative<Error>(result));
        QVERIFY(QFileInfo::exists(path));
        value.confirmedDestructive = true;
        successful(value);
        QVERIFY(!QFileInfo::exists(path));
    }
    void safePathRejections() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkpath("folder/child"));
        write(temp.filePath("folder/a"), "a");
        auto same = platform_->mutate(request(OperationKind::Copy, {temp.filePath("folder")}, temp.path()),
                                      {}, cancellation());
        QVERIFY(std::holds_alternative<Error>(same));
        QCOMPARE(std::get<Error>(same).code, ErrorCode::Conflict);
        auto inside = platform_->mutate(
            request(OperationKind::Move, {temp.filePath("folder")}, temp.filePath("folder/child")), {},
            cancellation());
        QVERIFY(std::holds_alternative<Error>(inside));
        auto overlap = platform_->mutate(request(OperationKind::Copy,
                                                 {temp.filePath("folder"), temp.filePath("folder/a")},
                                                 temp.filePath("folder/child")),
                                         {}, cancellation());
        QVERIFY(std::holds_alternative<Error>(overlap));
        QVERIFY(QFileInfo::exists(temp.filePath("folder/a")));
    }
    void cancellationRemovesOnlyOwnedStage() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("target"));
        const auto source = temp.filePath("source");
        write(source, QByteArray(400000, 's'));
        const auto expected = digest(source);
        const auto token = cancellation();
        const auto result = platform_->mutate(
            request(OperationKind::Copy, {source}, temp.filePath("target")),
            [&](const OperationProgress &state) {
                if (state.bytes > 0)
                    token->store(true);
            },
            token);
        QVERIFY(std::holds_alternative<OperationReport>(result));
        QVERIFY(std::get<OperationReport>(result).cancelled);
        QCOMPARE(digest(source), expected);
        QVERIFY(!QFileInfo::exists(temp.filePath("target/source")));
        QVERIFY(QDir(temp.filePath("target"))
                    .entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot)
                    .isEmpty());
    }
    void destinationAppearsDuringCopy() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("target"));
        const auto source = temp.filePath("source");
        write(source, "new");
        bool injected = false;
        const auto result = platform_->mutate(
            request(OperationKind::Copy, {source}, temp.filePath("target")),
            [&](const auto &state) {
                if (state.phase == "Committing" && !injected) {
                    injected = true;
                    write(temp.filePath("target/source"), "external");
                }
            },
            cancellation());
        QVERIFY(std::holds_alternative<OperationReport>(result));
        QVERIFY(!std::get<OperationReport>(result).success());
        QCOMPARE(digest(temp.filePath("target/source")),
                 QCryptographicHash::hash("external", QCryptographicHash::Sha256));
        QVERIFY(QFileInfo::exists(source));
    }
    void sourceChangesAfterPreflight() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("target"));
        const auto source = temp.filePath("source");
        write(source, "old");
        bool changed = false;
        const auto result = platform_->mutate(
            request(OperationKind::Move, {source}, temp.filePath("target")),
            [&](const auto &state) {
                if (state.phase == "Working" && !changed) {
                    changed = true;
                    write(source, "changed and longer");
                }
            },
            cancellation());
        QVERIFY(std::holds_alternative<OperationReport>(result));
        QVERIFY(!std::get<OperationReport>(result).success());
        QVERIFY(QFileInfo::exists(source));
        QVERIFY(!QFileInfo::exists(temp.filePath("target/source")));
    }
    void cycleMutationRejected() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("source"));
        QVERIFY(dir.mkdir("target"));
        std::error_code ec;
#ifdef Q_OS_WIN
        const auto source = std::filesystem::path(temp.filePath("source").toStdWString());
#else
        const auto source = std::filesystem::path(temp.filePath("source").toStdString());
#endif
        std::filesystem::create_directory_symlink(source, source / "cycle", ec);
        if (ec)
            QSKIP(qPrintable(QString::fromStdString(ec.message())));
        const auto result = platform_->mutate(
            request(OperationKind::Copy, {temp.filePath("source")}, temp.filePath("target")), {},
            cancellation());
        QVERIFY(std::holds_alternative<Error>(result));
        QCOMPARE(std::get<Error>(result).code, ErrorCode::Unsupported);
        std::filesystem::remove(source / "cycle", ec);
        QVERIFY(!ec);
        QVERIFY(QDir(temp.filePath("target")).isEmpty());
    }
    void asyncProgressAndShutdown() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("target"));
        const auto source = temp.filePath("source");
        write(source, QByteArray(2 * 1024 * 1024, 's'));
        OperationManager manager(platform_);
        QSignalSpy finished(&manager, &OperationManager::finished);
        QSignalSpy progress(&manager, &OperationManager::progressChanged);
        QVERIFY(manager.start(request(OperationKind::Copy, {source}, temp.filePath("target"))));
        QVERIFY(!manager.start(request(OperationKind::Copy, {source}, temp.filePath("target"))));
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(!manager.busy());
        QVERIFY(!progress.isEmpty());
        QCOMPARE(digest(source), digest(temp.filePath("target/source")));
        {
            OperationManager closing(platform_);
            auto value = request(OperationKind::Copy, {source}, temp.filePath("target"));
            value.collision = CollisionPolicy::Skip;
            closing.start(value);
        }
        QVERIFY(QFileInfo::exists(source));
    }
    void mixedItemResultsAndLateCancellation() {
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("target"));
        const auto first = temp.filePath("first"), second = temp.filePath("second");
        write(first, "first bytes");
        write(second, "second bytes");
        bool removed = false;
        const auto mixed = platform_->mutate(
            request(OperationKind::Copy, {first, second}, temp.filePath("target")),
            [&](const auto &state) {
                if (state.items == 1 && !removed) {
                    removed = true;
                    QFile::remove(second);
                }
            },
            cancellation());
        QVERIFY(std::holds_alternative<OperationReport>(mixed));
        const auto report = std::get<OperationReport>(mixed);
        QCOMPARE(report.items.size(), 2);
        QVERIFY(report.items[0].succeeded);
        QVERIFY(report.items[1].error);
        QVERIFY(!report.success());
        QVERIFY(QFileInfo::exists(first));
        QCOMPARE(digest(first), digest(temp.filePath("target/first")));
        QFile::remove(temp.filePath("target/first"));
        const auto token = cancellation();
        const auto late = platform_->mutate(
            request(OperationKind::Copy, {first}, temp.filePath("target")),
            [&](const auto &state) {
                if (state.phase == "Final cleanup")
                    token->store(true);
            },
            token);
        QVERIFY(std::holds_alternative<OperationReport>(late));
        QVERIFY(std::get<OperationReport>(late).items.front().succeeded);
        QCOMPARE(digest(first), digest(temp.filePath("target/first")));
        QVERIFY(QDir(temp.filePath("target"))
                    .entryList({".explorer-a-*"}, QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot)
                    .isEmpty());
    }
    void retainedCleanupOwnershipAndRetry() {
#ifdef Q_OS_WIN
        QTemporaryDir temp(ownedTemplate());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("target"));
        const auto source = temp.filePath("source");
        write(source, "source preserved");
        HANDLE locked = INVALID_HANDLE_VALUE;
        QString workspace;
        const auto result = platform_->mutate(
            request(OperationKind::Copy, {source}, temp.filePath("target")),
            [&](const auto &state) {
                if (state.phase == "Committing" && locked == INVALID_HANDLE_VALUE) {
                    const auto names = QDir(temp.filePath("target"))
                                           .entryList({".explorer-a-work-*"},
                                                      QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);
                    if (names.size() == 1) {
                        workspace = QDir(temp.filePath("target")).filePath(names.front());
                        const auto payload =
                            QDir::toNativeSeparators(QDir(workspace).filePath("payload")).toStdWString();
                        locked = CreateFileW(payload.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL, nullptr);
                    }
                }
            },
            cancellation());
        QVERIFY(locked != INVALID_HANDLE_VALUE);
        CloseHandle(locked);
        QVERIFY(std::holds_alternative<OperationReport>(result));
        const auto report = std::get<OperationReport>(result);
        QVERIFY(!report.success());
        QVERIFY(report.items.front().cleanupPaths.contains(workspace));
        QVERIFY(QFileInfo::exists(source));
        const auto moved = workspace + "-held";
        QVERIFY(QDir().rename(workspace, moved));
        QVERIFY(QDir().mkdir(workspace));
        write(QDir(workspace).filePath("foreign"), "replacement data");
        const auto foreign = platform_->retryCleanup(workspace);
        QVERIFY(std::holds_alternative<Error>(foreign));
        QCOMPARE(std::get<Error>(foreign).code, ErrorCode::Conflict);
        QVERIFY(QFileInfo::exists(QDir(workspace).filePath("foreign")));
        QVERIFY(QFile::remove(QDir(workspace).filePath("foreign")));
        QVERIFY(QDir().rmdir(workspace));
        QVERIFY(QDir().rename(moved, workspace));
        QVERIFY(std::holds_alternative<Success>(platform_->retryCleanup(workspace)));
        QVERIFY(!QFileInfo::exists(workspace));
        QVERIFY(std::holds_alternative<Error>(platform_->retryCleanup(temp.path())));
        QVERIFY(QFileInfo::exists(source));
#else
        QSKIP("Windows exclusive file sharing provides this real cleanup-failure fixture; Linux permission "
              "mutation is not authorized.");
#endif
    }
};
QTEST_GUILESS_MAIN(OperationsTest)
#include "test_operations.moc"
