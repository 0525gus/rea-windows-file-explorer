#include "explorer/platform.hpp"
#include "operation_sandbox.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QTemporaryDir>
#include <QTest>
#include <algorithm>
#include <cstddef>
#include <filesystem>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <cstring>
#include <winioctl.h>
// Keep Win32's A/W selector macro out of the domain's OperationKind names.
#undef CreateDirectory
#endif

using namespace explorer;

namespace {
#ifdef Q_OS_WIN
struct TestHandle {
    HANDLE value = INVALID_HANDLE_VALUE;
    ~TestHandle() {
        close();
    }
    void close() {
        if (value != INVALID_HANDLE_VALUE)
            CloseHandle(value);
        value = INVALID_HANDLE_VALUE;
    }
};
struct OwnedJunction {
    QString path;
    bool created = false;
    ~OwnedJunction() {
        // Remove the link itself before QTemporaryDir tears down its parent.
        if (created && !remove())
            qWarning().noquote() << "Test junction cleanup failed:" << path << "native:" << GetLastError();
    }
    bool remove() {
        if (!created)
            return true;
        if (!RemoveDirectoryW(reinterpret_cast<LPCWSTR>(path.utf16())))
            return false;
        created = false;
        return true;
    }
    DWORD create(const QString &target) {
        const auto nativeTarget = QDir::toNativeSeparators(QFileInfo(target).absoluteFilePath());
        const auto substitute = QStringLiteral("\\??\\") + nativeTarget;
        struct MountPoint {
            DWORD tag;
            WORD length, reserved;
            WORD substituteOffset, substituteLength, printOffset, printLength;
            wchar_t names[1];
        };
        const auto nameBytes = (substitute.size() + nativeTarget.size() + 2) * qsizetype(sizeof(wchar_t));
        QByteArray data(qsizetype(offsetof(MountPoint, names)) + nameBytes, '\0');
        auto *point = reinterpret_cast<MountPoint *>(data.data());
        point->tag = IO_REPARSE_TAG_MOUNT_POINT;
        point->length = WORD(data.size() - 8);
        point->substituteOffset = 0;
        point->substituteLength = WORD(substitute.size() * sizeof(wchar_t));
        point->printOffset = WORD((substitute.size() + 1) * sizeof(wchar_t));
        point->printLength = WORD(nativeTarget.size() * sizeof(wchar_t));
        std::memcpy(reinterpret_cast<char *>(point->names), substitute.utf16(), point->substituteLength);
        std::memcpy(reinterpret_cast<char *>(point->names) + point->printOffset, nativeTarget.utf16(),
                    point->printLength);
        if (!CreateDirectoryW(reinterpret_cast<LPCWSTR>(path.utf16()), nullptr))
            return GetLastError();
        created = true;
        TestHandle handle;
        handle.value =
            CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (handle.value == INVALID_HANDLE_VALUE)
            return GetLastError();
        DWORD returned = 0;
        if (!DeviceIoControl(handle.value, FSCTL_SET_REPARSE_POINT, data.data(), DWORD(data.size()), nullptr,
                             0, &returned, nullptr))
            return GetLastError();
        return ERROR_SUCCESS;
    }
};
#endif
QByteArray digest(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
        return {};
    return hash.result().toHex();
}

bool write(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::NewOnly) && file.write(bytes) == bytes.size() &&
           file.flush();
}

std::filesystem::path nativePath(const QString &path) {
#ifdef Q_OS_WIN
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toUtf8().constData());
#endif
}

struct Sandbox {
    QTemporaryDir directory;
    std::shared_ptr<IPlatform> platform = makePlatform();
    QString source, destination;
    Sandbox()
        : directory(operationSandboxTemplate(QStringLiteral("runtime-operations"))),
          source(directory.filePath(QStringLiteral("source"))),
          destination(directory.filePath(QStringLiteral("destination"))) {
        if (directory.isValid()) {
            QDir().mkpath(source);
            QDir().mkpath(destination);
        }
    }
    QString sourcePath(const QString &name) const {
        return QDir(source).filePath(name);
    }
    QString destinationPath(const QString &name) const {
        return QDir(destination).filePath(name);
    }
    Result<Location> location(const QString &path) const {
        return platform->parseLocation(path);
    }
    Result<Entry> entry(const QString &path) const {
        auto parsed = platform->parseLocation(QFileInfo(path).absolutePath());
        if (const auto *error = failure(parsed))
            return *error;
        auto result = platform->list(std::get<Location>(parsed));
        if (const auto *error = failure(result))
            return *error;
        for (const auto &entry : std::get<Listing>(result).entries)
            if (entry.name == QFileInfo(path).fileName())
                return entry;
        return Error{ErrorCode::NotFound, QStringLiteral("test fixture lookup"), path,
                     QStringLiteral("Test fixture item missing")};
    }
    QStringList descendants(const QString &root) const {
        QStringList result;
        std::error_code error;
        for (std::filesystem::recursive_directory_iterator it(nativePath(root), error), end;
             !error && it != end; it.increment(error)) {
            const auto relative = std::filesystem::relative(it->path(), nativePath(root), error);
            if (error)
                break;
#ifdef Q_OS_WIN
            result.push_back(QDir::fromNativeSeparators(QString::fromStdWString(relative.native())));
#else
            result.push_back(QString::fromUtf8(relative.native().c_str()));
#endif
        }
        if (error)
            return {QStringLiteral("<fixture-enumeration-failed>")};
        result.sort(Qt::CaseSensitive);
        return result;
    }
};

const ItemOutcome *outcomeFor(const OperationReport &report, const QString &path) {
    const auto found = std::find_if(report.items.begin(), report.items.end(),
                                    [&](const ItemOutcome &outcome) { return outcome.sourcePath == path; });
    return found == report.items.end() ? nullptr : &*found;
}
} // namespace

class OperationTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        QVERIFY2(!operationSandboxTemplate(QStringLiteral("runtime-operations")).isEmpty(),
                 "REA_OPERATION_TEST_ROOT must name an existing absolute directory.");
        qInfo().noquote()
            << "Operation sandbox parent:"
            << QFileInfo(operationSandboxTemplate(QStringLiteral("runtime-operations"))).absolutePath();
    }
    void emptySourceRequestIsExplicitlyRejected() {
        Sandbox s;
        QVERIFY(s.directory.isValid());
        const auto destination = s.location(s.destination);
        QVERIFY(!failure(destination));
        for (const auto kind :
             {OperationKind::Copy, OperationKind::Move, OperationKind::Recycle, OperationKind::Rename}) {
            const auto report =
                s.platform->execute({kind, {}, std::get<Location>(destination), QStringLiteral("unused")});
            QCOMPARE(report.items.size(), 1);
            QCOMPARE(report.items.first().status, ItemStatus::Failed);
            QVERIFY(report.items.first().error);
            QCOMPARE(report.items.first().error->code, ErrorCode::InvalidPath);
            QVERIFY(!report.cancelled && report.cleanupPending.isEmpty());
            QVERIFY(s.descendants(s.source).isEmpty());
            QVERIFY(s.descendants(s.destination).isEmpty());
        }
    }
    void copySingleAndMixedCollisionPreserveEverySource() {
        Sandbox s;
        QVERIFY(s.directory.isValid());
        QVERIFY(s.platform->capabilities().copy);
        QVERIFY(write(s.sourcePath(QStringLiteral("first.txt")), QByteArray("first-original")));
        QVERIFY(write(s.sourcePath(QStringLiteral("unique.txt")), QByteArray("unique-original")));
        const auto firstResult = s.entry(s.sourcePath(QStringLiteral("first.txt")));
        const auto uniqueResult = s.entry(s.sourcePath(QStringLiteral("unique.txt")));
        const auto destination = s.location(s.destination);
        QVERIFY(!failure(firstResult) && !failure(uniqueResult) && !failure(destination));
        const auto first = std::get<Entry>(firstResult), unique = std::get<Entry>(uniqueResult);
        QVERIFY(!first.identity.isEmpty() && !unique.identity.isEmpty());
        QVERIFY(!std::get<Location>(destination).identity.isEmpty());
        const auto firstHash = digest(first.path), uniqueHash = digest(unique.path);
        OperationRequest request{
            OperationKind::Copy, {first}, std::get<Location>(destination), {}, ConflictPolicy::Fail};
        const auto single = s.platform->execute(request);
        QCOMPARE(single.items.size(), 1);
        QCOMPARE(single.items.first().status, ItemStatus::Succeeded);
        QVERIFY(!single.cancelled);
        QVERIFY(single.cleanupPending.isEmpty());
        QCOMPARE(digest(s.destinationPath(QStringLiteral("first.txt"))), firstHash);
        QCOMPARE(digest(first.path), firstHash);

        request.sources = {first, unique};
        const auto mixed = s.platform->execute(request);
        QCOMPARE(mixed.items.size(), 2);
        const auto *conflict = outcomeFor(mixed, first.path);
        const auto *copied = outcomeFor(mixed, unique.path);
        QVERIFY(conflict && copied);
        QCOMPARE(conflict->status, ItemStatus::Failed);
        QVERIFY(conflict->error);
        QCOMPARE(conflict->error->code, ErrorCode::Conflict);
        QCOMPARE(copied->status, ItemStatus::Succeeded);
        QCOMPARE(digest(s.destinationPath(QStringLiteral("first.txt"))), firstHash);
        QCOMPARE(digest(s.destinationPath(QStringLiteral("unique.txt"))), uniqueHash);
        QCOMPARE(digest(first.path), firstHash);
        QCOMPARE(digest(unique.path), uniqueHash);
        QCOMPARE(s.descendants(s.destination),
                 (QStringList{QStringLiteral("first.txt"), QStringLiteral("unique.txt")}));
    }

    void keepBothReturnsRealNewDestinationWithoutOverwriting() {
        Sandbox s;
        QVERIFY(s.directory.isValid());
        QVERIFY(write(s.sourcePath(QStringLiteral("duplicate.txt")), QByteArray("incoming")));
        QVERIFY(write(s.destinationPath(QStringLiteral("duplicate.txt")), QByteArray("existing")));
        const auto source = s.entry(s.sourcePath(QStringLiteral("duplicate.txt")));
        const auto destination = s.location(s.destination);
        QVERIFY(!failure(source) && !failure(destination));
        const auto existingHash = digest(s.destinationPath(QStringLiteral("duplicate.txt")));
        const auto sourceHash = digest(std::get<Entry>(source).path);
        const auto report = s.platform->execute({OperationKind::Copy,
                                                 {std::get<Entry>(source)},
                                                 std::get<Location>(destination),
                                                 {},
                                                 ConflictPolicy::KeepBoth});
        QCOMPARE(report.items.size(), 1);
        const auto &item = report.items.first();
        QCOMPARE(item.status, ItemStatus::Succeeded);
        QVERIFY(!item.destinationPath.isEmpty());
        QVERIFY(QFileInfo(item.destinationPath).absoluteFilePath() !=
                QFileInfo(s.destinationPath(QStringLiteral("duplicate.txt"))).absoluteFilePath());
        QCOMPARE(QFileInfo(item.destinationPath).absolutePath(), QFileInfo(s.destination).absoluteFilePath());
        QCOMPARE(digest(item.destinationPath), sourceHash);
        QCOMPARE(digest(s.destinationPath(QStringLiteral("duplicate.txt"))), existingHash);
        QCOMPARE(digest(std::get<Entry>(source).path), sourceHash);
        QCOMPARE(s.descendants(s.destination).size(), 2);
    }

    void multiMoveReportsEachItemAndPreservesCollisionSource() {
        Sandbox s;
        QVERIFY(s.directory.isValid());
        QVector<Entry> sources;
        QMap<QString, QByteArray> sourceHashes;
        for (const auto &name :
             {QStringLiteral("one.txt"), QStringLiteral("blocked.txt"), QStringLiteral("three.txt")}) {
            QVERIFY(write(s.sourcePath(name), QByteArray("incoming-") + name.toUtf8()));
            const auto value = s.entry(s.sourcePath(name));
            QVERIFY(!failure(value));
            sources.push_back(std::get<Entry>(value));
            sourceHashes.insert(name, digest(s.sourcePath(name)));
        }
        QVERIFY(
            write(s.destinationPath(QStringLiteral("blocked.txt")), QByteArray("existing-do-not-overwrite")));
        const auto existingHash = digest(s.destinationPath(QStringLiteral("blocked.txt")));
        const auto destination = s.location(s.destination);
        QVERIFY(!failure(destination));
        const auto report =
            s.platform->execute({OperationKind::Move, sources, std::get<Location>(destination)});
        QCOMPARE(report.items.size(), 3);
        for (const auto &source : sources) {
            const auto *outcome = outcomeFor(report, source.path);
            QVERIFY(outcome);
            if (source.name == QStringLiteral("blocked.txt")) {
                QCOMPARE(outcome->status, ItemStatus::Failed);
                QVERIFY(outcome->error);
                QCOMPARE(outcome->error->code, ErrorCode::Conflict);
                QCOMPARE(digest(source.path), sourceHashes.value(source.name));
                QCOMPARE(digest(s.destinationPath(source.name)), existingHash);
            } else {
                QCOMPARE(outcome->status, ItemStatus::Succeeded);
                QVERIFY(!QFileInfo::exists(source.path));
                QCOMPARE(digest(s.destinationPath(source.name)), sourceHashes.value(source.name));
            }
        }
        QCOMPARE(s.descendants(s.source), QStringList{QStringLiteral("blocked.txt")});
        QCOMPARE(s.descendants(s.destination),
                 (QStringList{QStringLiteral("blocked.txt"), QStringLiteral("one.txt"),
                              QStringLiteral("three.txt")}));
    }

    void destinationAppearingDuringCopyIsNeverOverwritten() {
        Sandbox s;
        QVERIFY(s.directory.isValid());
        const auto sourcePath = s.sourcePath(QStringLiteral("race.bin"));
        const auto destinationPath = s.destinationPath(QStringLiteral("race.bin"));
        QVERIFY(write(sourcePath, QByteArray(8 * 1024 * 1024, 'R')));
        const auto source = s.entry(sourcePath);
        const auto destination = s.location(s.destination);
        QVERIFY(!failure(source) && !failure(destination));
        const auto before = digest(sourcePath);
        const QByteArray competingBytes("created-by-concurrent-actor");
        bool attempted = false, created = false;
        const auto report = s.platform->execute(
            {OperationKind::Copy, {std::get<Entry>(source)}, std::get<Location>(destination)}, {},
            [&](const OperationProgress &progress) {
                if (!attempted && progress.completedBytes > 0) {
                    attempted = true;
                    created = write(destinationPath, competingBytes);
                }
            });
        QVERIFY2(attempted && created,
                 "Create the competing destination after real transfer progress and before publication.");
        QCOMPARE(report.items.size(), 1);
        QCOMPARE(report.items.first().status, ItemStatus::Failed);
        QVERIFY(report.items.first().error);
        QCOMPARE(report.items.first().error->code, ErrorCode::Conflict);
        QCOMPARE(digest(sourcePath), before);
        QCOMPARE(digest(destinationPath),
                 QCryptographicHash::hash(competingBytes, QCryptographicHash::Sha256).toHex());
        QCOMPARE(s.descendants(s.destination), QStringList{QStringLiteral("race.bin")});
        QVERIFY(report.cleanupPending.isEmpty());
    }

    void folderCopyMoveAndDescendantDestinationGuard() {
        Sandbox s;
        QVERIFY(s.directory.isValid());
        QVERIFY(QDir().mkpath(s.sourcePath(QStringLiteral("tree/nested"))));
        QVERIFY(write(s.sourcePath(QStringLiteral("tree/root.txt")), QByteArray("root")));
        QVERIFY(write(s.sourcePath(QStringLiteral("tree/nested/leaf.txt")), QByteArray("nested-content")));
        const auto treeResult = s.entry(s.sourcePath(QStringLiteral("tree")));
        const auto destination = s.location(s.destination);
        QVERIFY(!failure(treeResult) && !failure(destination));
        const auto tree = std::get<Entry>(treeResult);
        QVERIFY(tree.directory && !tree.link && !tree.identity.isEmpty());
        const auto beforeNames = s.descendants(s.source);
        const auto rootHash = digest(s.sourcePath(QStringLiteral("tree/root.txt")));
        const auto leafHash = digest(s.sourcePath(QStringLiteral("tree/nested/leaf.txt")));
        const auto copied =
            s.platform->execute({OperationKind::Copy, {tree}, std::get<Location>(destination)});
        QCOMPARE(copied.items.size(), 1);
        QCOMPARE(copied.items.first().status, ItemStatus::Succeeded);
        QCOMPARE(digest(s.destinationPath(QStringLiteral("tree/root.txt"))), rootHash);
        QCOMPARE(digest(s.destinationPath(QStringLiteral("tree/nested/leaf.txt"))), leafHash);
        QCOMPARE(s.descendants(s.source), beforeNames);

        const auto inside = s.location(s.sourcePath(QStringLiteral("tree/nested")));
        QVERIFY(!failure(inside));
        const auto rejected = s.platform->execute({OperationKind::Copy, {tree}, std::get<Location>(inside)});
        QCOMPARE(rejected.items.size(), 1);
        QCOMPARE(rejected.items.first().status, ItemStatus::Failed);
        QVERIFY(rejected.items.first().error);
        QCOMPARE(rejected.items.first().error->code, ErrorCode::InvalidPath);
        QCOMPARE(s.descendants(s.source), beforeNames);

        QVERIFY(QDir().mkpath(s.directory.filePath(QStringLiteral("moved"))));
        const auto moveDestination = s.location(s.directory.filePath(QStringLiteral("moved")));
        QVERIFY(!failure(moveDestination));
        const auto moved =
            s.platform->execute({OperationKind::Move, {tree}, std::get<Location>(moveDestination)});
        QCOMPARE(moved.items.size(), 1);
        QCOMPARE(moved.items.first().status, ItemStatus::Succeeded);
        QVERIFY(!QFileInfo::exists(tree.path));
        QCOMPARE(digest(s.directory.filePath(QStringLiteral("moved/tree/root.txt"))), rootHash);
        QCOMPARE(digest(s.directory.filePath(QStringLiteral("moved/tree/nested/leaf.txt"))), leafHash);
        QCOMPARE(digest(s.destinationPath(QStringLiteral("tree/nested/leaf.txt"))), leafHash);
    }

    void renameCollisionInvalidLeafAndCreateDirectory() {
        Sandbox s;
        QVERIFY(s.directory.isValid());
        QVERIFY(write(s.sourcePath(QStringLiteral("original.txt")), QByteArray("original")));
        QVERIFY(write(s.sourcePath(QStringLiteral("taken.txt")), QByteArray("taken")));
        const auto sourceResult = s.entry(s.sourcePath(QStringLiteral("original.txt")));
        const auto parent = s.location(s.source);
        QVERIFY(!failure(sourceResult) && !failure(parent));
        const auto original = std::get<Entry>(sourceResult);
        const auto parentValue = std::get<Location>(parent);
        const auto originalHash = digest(original.path),
                   existingHash = digest(s.sourcePath(QStringLiteral("taken.txt")));
        const auto collision = s.platform->execute(
            {OperationKind::Rename, {original}, parentValue, QStringLiteral("taken.txt")});
        QCOMPARE(collision.items.size(), 1);
        QCOMPARE(collision.items.first().status, ItemStatus::Failed);
        QVERIFY(collision.items.first().error);
        QCOMPARE(collision.items.first().error->code, ErrorCode::Conflict);
        QCOMPARE(digest(original.path), originalHash);
        QCOMPARE(digest(s.sourcePath(QStringLiteral("taken.txt"))), existingHash);
        for (const auto &leaf :
             {QStringLiteral("../escape"), QStringLiteral("sub/path"), QStringLiteral(".."), QString{}}) {
            const auto invalid = s.platform->execute({OperationKind::Rename, {original}, parentValue, leaf});
            QCOMPARE(invalid.items.size(), 1);
            QCOMPARE(invalid.items.first().status, ItemStatus::Failed);
            QVERIFY(invalid.items.first().error);
            QCOMPARE(invalid.items.first().error->code, ErrorCode::InvalidPath);
            QCOMPARE(digest(original.path), originalHash);
        }
        const auto renamed = s.platform->execute(
            {OperationKind::Rename, {original}, parentValue, QString::fromUtf8("새 이름.txt")});
        QCOMPARE(renamed.items.size(), 1);
        QCOMPARE(renamed.items.first().status, ItemStatus::Succeeded);
        QVERIFY(!QFileInfo::exists(original.path));
        QCOMPARE(digest(s.sourcePath(QString::fromUtf8("새 이름.txt"))), originalHash);
        const auto created = s.platform->execute(
            {OperationKind::CreateDirectory, {}, parentValue, QString::fromUtf8("새 폴더")});
        QCOMPARE(created.items.size(), 1);
        QCOMPARE(created.items.first().status, ItemStatus::Succeeded);
        QVERIFY(QFileInfo(s.sourcePath(QString::fromUtf8("새 폴더"))).isDir());
        const auto duplicate = s.platform->execute(
            {OperationKind::CreateDirectory, {}, parentValue, QString::fromUtf8("새 폴더")});
        QCOMPARE(duplicate.items.size(), 1);
        QCOMPARE(duplicate.items.first().status, ItemStatus::Failed);
        QVERIFY(duplicate.items.first().error);
        QCOMPARE(duplicate.items.first().error->code, ErrorCode::Conflict);
        QCOMPARE(digest(s.sourcePath(QStringLiteral("taken.txt"))), existingHash);
    }

    void staleSourceAndDestinationIdentitiesRefuseMutation() {
        Sandbox s;
        QVERIFY(s.directory.isValid());
        QVERIFY(write(s.sourcePath(QStringLiteral("item.txt")), QByteArray("source-before")));
        auto sourceResult = s.entry(s.sourcePath(QStringLiteral("item.txt")));
        auto destinationResult = s.location(s.destination);
        QVERIFY(!failure(sourceResult) && !failure(destinationResult));
        const auto oldSource = std::get<Entry>(sourceResult);
        const auto oldDestination = std::get<Location>(destinationResult);
        QVERIFY(!oldSource.identity.isEmpty() && !oldDestination.identity.isEmpty());
        QVERIFY(
            write(s.sourcePath(QStringLiteral("replacement.txt")), QByteArray("replacement-before-rename")));
        const auto replacement = s.entry(s.sourcePath(QStringLiteral("replacement.txt")));
        QVERIFY(!failure(replacement));
        QVERIFY(std::get<Entry>(replacement).identity != oldSource.identity);
        QVERIFY(QFile::remove(oldSource.path));
        QVERIFY(QFile::rename(s.sourcePath(QStringLiteral("replacement.txt")), oldSource.path));
        const auto staleSource = s.platform->execute({OperationKind::Copy, {oldSource}, oldDestination});
        QCOMPARE(staleSource.items.size(), 1);
        QCOMPARE(staleSource.items.first().status, ItemStatus::Failed);
        QVERIFY(staleSource.items.first().error);
        QCOMPARE(staleSource.items.first().error->code, ErrorCode::Changed);
        QVERIFY(s.descendants(s.destination).isEmpty());
        const auto newSource = s.entry(oldSource.path);
        QVERIFY(!failure(newSource));
        const auto preservedHash = digest(oldSource.path);

        const auto replacementDirectory = s.directory.filePath(QStringLiteral("new-destination"));
        QVERIFY(QDir().mkpath(replacementDirectory));
        const auto newDirectory = s.location(replacementDirectory);
        QVERIFY(!failure(newDirectory));
        QVERIFY(std::get<Location>(newDirectory).identity != oldDestination.identity);
        QVERIFY(QDir().rename(s.destination, s.directory.filePath(QStringLiteral("old-destination"))));
        QVERIFY(QDir().rename(replacementDirectory, s.destination));
        const auto staleDestination =
            s.platform->execute({OperationKind::Move, {std::get<Entry>(newSource)}, oldDestination});
        QCOMPARE(staleDestination.items.size(), 1);
        QCOMPARE(staleDestination.items.first().status, ItemStatus::Failed);
        QVERIFY(staleDestination.items.first().error);
        QCOMPARE(staleDestination.items.first().error->code, ErrorCode::Changed);
        QCOMPARE(digest(oldSource.path), preservedHash);
        QVERIFY(s.descendants(s.destination).isEmpty());
        QVERIFY(s.descendants(s.directory.filePath(QStringLiteral("old-destination"))).isEmpty());
    }

    void copyCancellationPreservesOriginalAndAccountsForOwnedResidue() {
        Sandbox s;
        QVERIFY(s.directory.isValid());
        const auto path = s.sourcePath(QStringLiteral("large.bin"));
        {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::NewOnly));
            const QByteArray block(1024 * 1024, 'Q');
            for (int i = 0; i < 16; ++i)
                QCOMPARE(file.write(block), qint64(block.size()));
            QVERIFY(file.flush());
        }
        const auto before = digest(path);
        const auto source = s.entry(path);
        const auto destination = s.location(s.destination);
        QVERIFY(!failure(source) && !failure(destination));
        std::stop_source stop;
        QVector<OperationProgress> progress;
        const auto report = s.platform->execute(
            {OperationKind::Copy, {std::get<Entry>(source)}, std::get<Location>(destination)},
            stop.get_token(), [&](const OperationProgress &value) {
                progress.push_back(value);
                if (value.completedBytes > 0)
                    stop.request_stop();
            });
        QVERIFY2(stop.stop_requested(),
                 "A real byte-progress callback must permit cancellation during transfer.");
        QVERIFY(report.cancelled);
        QCOMPARE(report.items.size(), 1);
        QVERIFY(report.items.first().status == ItemStatus::Cancelled ||
                report.items.first().status == ItemStatus::Partial);
        QCOMPARE(digest(path), before);
        QVERIFY(!QFileInfo::exists(s.destinationPath(QStringLiteral("large.bin"))));
        for (qsizetype i = 1; i < progress.size(); ++i)
            QVERIFY(progress[i].completedBytes >= progress[i - 1].completedBytes);
        const auto residues = s.descendants(s.destination);
        QSet<QString> accounted;
        for (const auto &value : report.cleanupPending)
            accounted.insert(QDir::cleanPath(value));
        for (const auto &value : report.items.first().retainedPaths)
            accounted.insert(QDir::cleanPath(value));
        for (const auto &relative : residues) {
            const auto full = QDir::cleanPath(s.destinationPath(relative));
            QVERIFY2(accounted.contains(full),
                     qPrintable(QStringLiteral("Unreported owned residue: ") + full));
            const auto cleanup = s.platform->retryCleanup(full);
            QVERIFY2(!failure(cleanup), failure(cleanup) ? qPrintable(failure(cleanup)->message) : "");
        }
        QVERIFY(s.descendants(s.destination).isEmpty());
    }

    void preCancellationAndForeignCleanupNeverDeleteUserSelectedData() {
        Sandbox s;
        QVERIFY(s.directory.isValid());
        QVERIFY(write(s.sourcePath(QStringLiteral("source.txt")), QByteArray("source")));
        const auto source = s.entry(s.sourcePath(QStringLiteral("source.txt")));
        const auto destination = s.location(s.destination);
        QVERIFY(!failure(source) && !failure(destination));
        const auto sourceHash = digest(std::get<Entry>(source).path);
        std::stop_source stop;
        stop.request_stop();
        const auto report = s.platform->execute(
            {OperationKind::Copy, {std::get<Entry>(source)}, std::get<Location>(destination)},
            stop.get_token());
        QVERIFY(report.cancelled);
        QCOMPARE(report.items.size(), 1);
        QCOMPARE(report.items.first().status, ItemStatus::Cancelled);
        QCOMPARE(digest(std::get<Entry>(source).path), sourceHash);
        QVERIFY(s.descendants(s.destination).isEmpty());
        const auto foreign = s.destinationPath(QStringLiteral(".rea-part-looks-owned-but-is-not"));
        QVERIFY(write(foreign, QByteArray("foreign-to-operation-manager")));
        const auto foreignHash = digest(foreign);
        const auto rejected = s.platform->retryCleanup(foreign);
        QVERIFY(failure(rejected));
        QVERIFY(!failure(rejected)->message.isEmpty());
        QCOMPARE(digest(foreign), foreignHash);
    }

    void windowsRealCleanupFailureRetainsOwnershipAndRefusesReplacement() {
#ifndef Q_OS_WIN
        QSKIP("Windows delete-sharing failure uses a real owned Win32 handle; no simulated OS result.");
#else
        Sandbox s;
        QVERIFY(s.directory.isValid());
        const auto sourcePath = s.sourcePath(QStringLiteral("large-owned.bin"));
        QVERIFY(write(sourcePath, QByteArray(8 * 1024 * 1024, 'L')));
        const auto source = s.entry(sourcePath);
        const auto destination = s.location(s.destination);
        QVERIFY(!failure(source) && !failure(destination));
        const auto sourceHash = digest(sourcePath);
        TestHandle denyDelete;
        QString stagedPath;
        DWORD lockError = ERROR_SUCCESS;
        std::stop_source stop;
        const auto report = s.platform->execute(
            {OperationKind::Copy, {std::get<Entry>(source)}, std::get<Location>(destination)},
            stop.get_token(), [&](const OperationProgress &progress) {
                if (!stagedPath.isEmpty() || progress.completedBytes == 0 ||
                    progress.phase != QStringLiteral("Committing without replacement"))
                    return;
                // The public commit milestone occurs after copy handles have
                // closed. Its exact staged path must stay in this owned tree.
                stagedPath = progress.currentPath;
                if (QFileInfo(stagedPath).absolutePath() != QFileInfo(s.destination).absoluteFilePath()) {
                    lockError = ERROR_INVALID_NAME;
                    stop.request_stop();
                    return;
                }
                denyDelete.value = CreateFileW(reinterpret_cast<LPCWSTR>(stagedPath.utf16()), GENERIC_READ,
                                               FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                               FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
                if (denyDelete.value == INVALID_HANDLE_VALUE)
                    lockError = GetLastError();
                stop.request_stop();
            });
        QVERIFY2(
            !stagedPath.isEmpty() && denyDelete.value != INVALID_HANDLE_VALUE,
            qPrintable(QStringLiteral("Real stage delete-sharing lock failed, native %1").arg(lockError)));
        QVERIFY(report.cancelled);
        QCOMPARE(report.items.size(), 1);
        QCOMPARE(report.items.first().status, ItemStatus::Partial);
        QVERIFY(report.items.first().error);
        QVERIFY(report.cleanupPending.contains(stagedPath));
        QVERIFY(report.items.first().retainedPaths.contains(stagedPath));
        QVERIFY(QFileInfo::exists(stagedPath));
        QCOMPARE(digest(sourcePath), sourceHash);
        QVERIFY(!QFileInfo::exists(s.destinationPath(QStringLiteral("large-owned.bin"))));
        const auto lockedRetry = s.platform->retryCleanup(stagedPath);
        QVERIFY(failure(lockedRetry));
        QVERIFY(failure(lockedRetry)->nativeCode != 0);
        QVERIFY(QFileInfo::exists(stagedPath));
        qInfo().noquote() << "Real cleanup retained:" << stagedPath
                          << "native:" << failure(lockedRetry)->nativeCode
                          << "reason:" << failure(lockedRetry)->message;
        denyDelete.close();

        const auto replacementPath = s.directory.filePath(QStringLiteral("foreign-replacement.bin"));
        const auto savedOwnedPath = s.directory.filePath(QStringLiteral("saved-owned-stage.bin"));
        QVERIFY(write(replacementPath, QByteArray("foreign replacement must never be deleted")));
        const auto stageEntry = s.platform->inspectEntry(stagedPath);
        const auto replacementEntry = s.platform->inspectEntry(replacementPath);
        QVERIFY(!failure(stageEntry) && !failure(replacementEntry));
        QVERIFY(std::get<Entry>(stageEntry).identity != std::get<Entry>(replacementEntry).identity);
        const auto replacementHash = digest(replacementPath);
        QVERIFY(QFile::rename(stagedPath, savedOwnedPath));
        QVERIFY(QFile::rename(replacementPath, stagedPath));
        const auto foreignRetry = s.platform->retryCleanup(stagedPath);
        QVERIFY(failure(foreignRetry));
        QCOMPARE(failure(foreignRetry)->code, ErrorCode::Changed);
        QCOMPARE(digest(stagedPath), replacementHash);
        QVERIFY(QFileInfo::exists(savedOwnedPath));
        qInfo().noquote() << "Cleanup refused changed identity at:" << stagedPath;
        QVERIFY(QFile::rename(stagedPath, replacementPath));
        QVERIFY(QFile::rename(savedOwnedPath, stagedPath));
        const auto completed = s.platform->retryCleanup(stagedPath);
        QVERIFY2(!failure(completed), failure(completed) ? qPrintable(failure(completed)->message) : "");
        QVERIFY(!QFileInfo::exists(stagedPath));
        QCOMPARE(digest(replacementPath), replacementHash);
        QCOMPARE(digest(sourcePath), sourceHash);
        QVERIFY(s.descendants(s.destination).isEmpty());
        QVERIFY(failure(s.platform->retryCleanup(stagedPath))); // Ownership was discharged, not path-based.
#endif
    }

    void windowsJunctionIsRejectedWithoutFollowingOwnedTarget_data() {
        QTest::addColumn<bool>("nested");
        QTest::newRow("selected-directory-junction") << false;
        QTest::newRow("junction-cycle-inside-selected-tree") << true;
    }
    void windowsJunctionIsRejectedWithoutFollowingOwnedTarget() {
        QFETCH(bool, nested);
#ifndef Q_OS_WIN
        QSKIP("Windows junction/reparse-point fixture is distinct from POSIX symlinks.");
#else
        Sandbox s;
        QVERIFY(s.directory.isValid());
        const auto target = nested ? s.sourcePath(QStringLiteral("tree"))
                                   : s.directory.filePath(QStringLiteral("junction-target"));
        QVERIFY(QDir().mkpath(target));
        const auto targetFile = QDir(target).filePath(QStringLiteral("preserve.txt"));
        QVERIFY(write(targetFile, QByteArray("junction target is outside selected tree")));
        const auto targetHash = digest(targetFile);
        OwnedJunction junction{nested ? QDir(target).filePath(QStringLiteral("cycle"))
                                      : s.sourcePath(QStringLiteral("junction"))};
        const auto error = junction.create(target);
        if (error == ERROR_NOT_SUPPORTED || error == ERROR_INVALID_FUNCTION ||
            error == ERROR_PRIVILEGE_NOT_HELD)
            QSKIP(qPrintable(QStringLiteral("Real junction fixture unavailable, native %1").arg(error)));
        QCOMPARE(error, DWORD(ERROR_SUCCESS));
        const auto attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(junction.path.utf16()));
        QVERIFY(attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT));
        // It must be a usable real junction, rather than an ordinary directory
        // whose creation alone could yield a misleading Unsupported pass.
        QCOMPARE(digest(QDir(junction.path).filePath(QStringLiteral("preserve.txt"))), targetHash);
        const auto link = s.platform->inspectEntry(junction.path);
        QVERIFY(!failure(link));
        QVERIFY(std::get<Entry>(link).link);
        const auto source = s.platform->inspectEntry(nested ? target : junction.path);
        const auto destination = s.location(s.destination);
        QVERIFY(!failure(source) && !failure(destination));
        const auto report = s.platform->execute(
            {OperationKind::Copy, {std::get<Entry>(source)}, std::get<Location>(destination)});
        QCOMPARE(report.items.size(), 1);
        QCOMPARE(report.items.first().status, ItemStatus::Failed);
        QVERIFY(report.items.first().error);
        QCOMPARE(report.items.first().error->code, ErrorCode::Unsupported);
        QCOMPARE(digest(targetFile), targetHash);
        QVERIFY(QFileInfo::exists(junction.path));
        QVERIFY(s.descendants(s.destination).isEmpty());
        QVERIFY2(junction.remove(), "Remove only the owned junction itself before temporary-tree teardown.");
        QCOMPARE(digest(targetFile), targetHash);
#endif
    }

    void copySymlinkMustNotFollowTarget_data() {
        QTest::addColumn<bool>("nested");
        QTest::newRow("selected-symlink") << false;
        QTest::newRow("link-inside-selected-directory") << true;
    }
    void copySymlinkMustNotFollowTarget() {
        QFETCH(bool, nested);
        Sandbox s;
        QVERIFY(s.directory.isValid());
        QVERIFY(QDir().mkpath(s.sourcePath(QStringLiteral("tree"))));
        const auto target = s.sourcePath(QStringLiteral("target.txt"));
        QVERIFY(write(target, QByteArray("link-target-must-stay")));
        const auto linkPath = s.sourcePath(nested ? QStringLiteral("tree/link") : QStringLiteral("link"));
        std::error_code error;
        std::filesystem::create_symlink(nativePath(target), nativePath(linkPath), error);
        if (error)
            QSKIP(qPrintable(QStringLiteral("Symlink fixture unavailable: ") +
                             QString::fromStdString(error.message())));
        const auto source = s.entry(nested ? s.sourcePath(QStringLiteral("tree")) : linkPath);
        const auto destination = s.location(s.destination);
        QVERIFY(!failure(source) && !failure(destination));
        if (!nested)
            QVERIFY(std::get<Entry>(source).link);
        const auto targetHash = digest(target);
        const auto report = s.platform->execute(
            {OperationKind::Copy, {std::get<Entry>(source)}, std::get<Location>(destination)});
        QCOMPARE(report.items.size(), 1);
        QCOMPARE(report.items.first().status, ItemStatus::Failed);
        QVERIFY(report.items.first().error);
        QCOMPARE(report.items.first().error->code, ErrorCode::Unsupported);
        QCOMPARE(digest(target), targetHash);
        QVERIFY(s.descendants(s.destination).isEmpty());
    }

    void recycleOnlyExplicitlyOptedInDisposableItem() {
        if (qEnvironmentVariable("REA_TEST_RECYCLE_OWNED") != QStringLiteral("1"))
            QSKIP("Real recycle is opt-in: REA_TEST_RECYCLE_OWNED=1; default test run does not add to the "
                  "user's bin.");
        Sandbox s;
        QVERIFY(s.directory.isValid());
        if (!s.platform->capabilities().recycle)
            QSKIP("Platform does not guarantee recycle capability; permanent-delete fallback is forbidden.");
        const auto owned = s.sourcePath(QStringLiteral("test-owned-recycle.txt"));
        const auto neighbor = s.sourcePath(QStringLiteral("never-recycle-neighbor.txt"));
        QVERIFY(write(owned, QByteArray("exclusive disposable recycle fixture")));
        QVERIFY(write(neighbor, QByteArray("preserved")));
        const auto source = s.entry(owned);
        QVERIFY(!failure(source));
        qInfo().noquote() << "Opt-in OS recycle fixture:" << owned;
        const auto report = s.platform->execute({OperationKind::Recycle, {std::get<Entry>(source)}});
        QCOMPARE(report.items.size(), 1);
        QCOMPARE(report.items.first().status, ItemStatus::Succeeded);
        QVERIFY(!QFileInfo::exists(owned));
        QCOMPARE(digest(neighbor),
                 QCryptographicHash::hash(QByteArray("preserved"), QCryptographicHash::Sha256).toHex());
        // Adapter's guaranteed recycle contract is exercised; OS bin contents
        // are not enumerated and permanent deletion is never requested.
    }
};

QTEST_GUILESS_MAIN(OperationTests)
#include "operation_tests.moc"
