#include "explorer/operations.h"
#include "filesystem_detail.h"
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <QUuid>
#include <limits>
#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(Q_OS_MACOS)
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#endif
#ifndef Q_OS_WIN
#include <sys/stat.h>
#endif

namespace explorer {
namespace {
using namespace filesystem_detail;
struct Node {
    QString source;
    QString relative;
    bool directory = false;
    qint64 size = 0;
    QDateTime modified;
};
struct Prepared {
    QString source;
    QString destination;
    bool directory = false;
    bool collision = false;
    QVector<Node> nodes;
};
Error problem(ErrorCode code, const QString &path, const QString &detail) {
    return {code, QStringLiteral("File operation"), path, detail, 0};
}
// The registry contains only directories this process atomically created. A held
// native directory handle prevents identity reuse while a cleanup is pending.
class DirectoryLease {
  public:
    explicit DirectoryLease(const QString &path) {
#ifdef Q_OS_WIN
        handle_ = CreateFileW(nativePath(path).c_str(), FILE_READ_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        BY_HANDLE_FILE_INFORMATION info{};
        if (handle_ != INVALID_HANDLE_VALUE && GetFileInformationByHandle(handle_, &info) &&
            (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            !(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            identity_ = QStringLiteral("%1:%2:%3")
                            .arg(info.dwVolumeSerialNumber)
                            .arg(info.nFileIndexHigh)
                            .arg(info.nFileIndexLow);
#else
        handle_ = ::open(nativePath(path).c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        struct stat info{};
        if (handle_ >= 0 && fstat(handle_, &info) == 0 && S_ISDIR(info.st_mode))
            identity_ = QStringLiteral("%1:%2").arg(qulonglong(info.st_dev)).arg(qulonglong(info.st_ino));
#endif
    }
    ~DirectoryLease() {
#ifdef Q_OS_WIN
        if (handle_ != INVALID_HANDLE_VALUE)
            CloseHandle(handle_);
#else
        if (handle_ >= 0)
            ::close(handle_);
#endif
    }
    bool valid() const {
        return !identity_.isEmpty();
    }
    bool matches(const QString &path) const {
        DirectoryLease current(path);
        return valid() && current.valid() && identity_ == current.identity_;
    }

  private:
    QString identity_;
#ifdef Q_OS_WIN
    HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
    int handle_ = -1;
#endif
};
QMutex cleanupMutex;
QHash<QString, std::shared_ptr<DirectoryLease>> ownedDirectories;
std::optional<Error> rememberOwned(const QString &path) {
    auto lease = std::make_shared<DirectoryLease>(path);
    if (!lease->valid())
        return problem(
            ErrorCode::Io, path,
            "Created workspace identity could not be established. It was retained for manual recovery.");
    QMutexLocker lock(&cleanupMutex);
    ownedDirectories.insert(path, std::move(lease));
    return {};
}
Result<bool> exists(const QString &path) {
    std::error_code ec;
    const auto status = fs::symlink_status(nativePath(path), ec);
    if (ec == std::errc::no_such_file_or_directory)
        return false;
    if (ec)
        return translate(ec, path, QStringLiteral("Inspect path"));
    return fs::exists(status);
}
QString key(QString path) {
    path = QDir::cleanPath(path);
#ifdef Q_OS_WIN
    path = path.toCaseFolded();
#endif
    return path;
}
bool inside(const QString &child, const QString &parent) {
    return key(child).startsWith(key(parent) + '/');
}
Result<QString> identity(const QString &path) {
    std::error_code ec;
    const auto resolved = fs::canonical(nativePath(path), ec);
    if (ec)
        return translate(ec, path, QStringLiteral("Resolve path"));
    return textPath(resolved);
}
bool validName(const QString &name) {
    if (name.isEmpty() || name == "." || name == ".." || name.contains('/') || name.contains(QChar(0)))
        return false;
#ifdef Q_OS_WIN
    if (name.contains('\\') || name.endsWith('.') || name.endsWith(' '))
        return false;
    for (const auto character : name)
        if (character.unicode() < 32 || QStringLiteral(":*?\"<>|").contains(character))
            return false;
    const auto base = name.section('.', 0, 0).toUpper();
    if (QStringList{"CON", "PRN", "AUX", "NUL"}.contains(base))
        return false;
    if ((base.startsWith("COM") || base.startsWith("LPT")) && base.size() == 4 && base.back() >= '1' &&
        base.back() <= '9')
        return false;
#endif
    return true;
}
std::optional<Error> appendNode(Prepared &item, const QString &path, const QString &relative, qint64 &total) {
    if (isReparse(path))
        return problem(ErrorCode::Unsupported, path,
                       "Link/reparse-point mutation is not supported; no link is followed.");
    std::error_code ec;
    const auto status = fs::status(nativePath(path), ec);
    if (ec)
        return translate(ec, path, "Inspect source");
    if (!fs::is_regular_file(status) && !fs::is_directory(status))
        return problem(ErrorCode::Unsupported, path, "Only regular files and directories are supported.");
    const QFileInfo info(path);
    const auto size = fs::is_directory(status) ? 0 : info.size();
    if (size < 0 || total > std::numeric_limits<qint64>::max() - size ||
        item.nodes.size() >= std::numeric_limits<int>::max())
        return problem(ErrorCode::Unsupported, path,
                       "The operation exceeds the representable item or byte count.");
    total += size;
    item.nodes.push_back({path, relative, fs::is_directory(status), size, info.lastModified()});
    return {};
}
Result<QVector<Prepared>> prepare(const OperationRequest &request, qint64 &total, const CancelToken &cancel) {
    if ((request.kind == OperationKind::Remove || request.collision == CollisionPolicy::Replace) &&
        !request.confirmedDestructive)
        return problem(ErrorCode::AccessDenied, {},
                       "Explicit confirmation is required for permanent deletion or replacement.");
    const bool named = request.kind == OperationKind::Rename || request.kind == OperationKind::NewFolder;
    if (named && !validName(request.name))
        return problem(ErrorCode::InvalidName, request.name, "Enter one valid leaf name without a path.");
    if (request.kind == OperationKind::Rename && request.sources.size() != 1)
        return problem(ErrorCode::InvalidName, {}, "Rename requires exactly one item.");
    if (request.kind != OperationKind::NewFolder && request.sources.isEmpty())
        return problem(ErrorCode::InvalidName, {}, "Select at least one source item.");
    QVector<Prepared> items;
    QString parent;
    if (request.kind == OperationKind::Copy || request.kind == OperationKind::Move ||
        request.kind == OperationKind::NewFolder) {
        const auto resolved = identity(request.destination);
        if (const auto *error = std::get_if<Error>(&resolved))
            return *error;
        parent = std::get<QString>(resolved);
        if (!QFileInfo(parent).isDir())
            return problem(ErrorCode::NotDirectory, request.destination, "Destination is not a directory.");
    }
    if (request.kind == OperationKind::NewFolder) {
        const auto destination = QDir(parent).filePath(request.name);
        const auto present = exists(destination);
        if (const auto *error = std::get_if<Error>(&present))
            return *error;
        if (std::get<bool>(present))
            return problem(ErrorCode::AlreadyExists, destination, "An item with this name already exists.");
        items.push_back({{}, destination, true, false, {}});
        return items;
    }
    QSet<QString> destinations;
    QStringList sourceIdentities;
    for (const auto &original : request.sources) {
        if (cancel->load())
            return cancelled(original);
        const auto source = QDir::cleanPath(QFileInfo(original).absoluteFilePath());
        if (QDir(source).isRoot())
            return problem(ErrorCode::AccessDenied, source, "Filesystem roots cannot be mutated.");
        if (isReparse(source))
            return problem(ErrorCode::Unsupported, source, "Link/reparse-point mutation is not supported.");
        const auto resolved = identity(source);
        if (const auto *error = std::get_if<Error>(&resolved))
            return *error;
        const auto canonical = std::get<QString>(resolved);
        for (const auto &other : sourceIdentities)
            if (key(canonical) == key(other) || inside(canonical, other) || inside(other, canonical))
                return problem(ErrorCode::Conflict, source,
                               "Selected sources overlap; select each item only once.");
        sourceIdentities.push_back(canonical);
        Prepared item;
        item.source = source;
        item.directory = QFileInfo(source).isDir();
        if (request.kind == OperationKind::Rename)
            item.destination = QDir(QFileInfo(source).absolutePath()).filePath(request.name);
        else if (request.kind == OperationKind::Copy || request.kind == OperationKind::Move)
            item.destination = QDir(parent).filePath(QFileInfo(source).fileName());
        if (!item.destination.isEmpty()) {
            if (key(canonical) == key(item.destination) ||
                (item.directory && inside(item.destination, canonical)))
                return problem(
                    ErrorCode::Conflict, item.destination,
                    "Source and destination are the same or the destination is inside the source.");
            if (destinations.contains(key(item.destination)))
                return problem(ErrorCode::Conflict, item.destination,
                               "Multiple selected sources have the same destination name.");
            destinations.insert(key(item.destination));
            const auto present = exists(item.destination);
            if (const auto *error = std::get_if<Error>(&present))
                return *error;
            item.collision = std::get<bool>(present);
            if (item.collision) {
                std::error_code sameError;
                const bool same = fs::equivalent(nativePath(source), nativePath(item.destination), sameError);
                if (sameError)
                    return translate(sameError, item.destination, "Compare source and destination identity");
                if (same)
                    return problem(ErrorCode::Conflict, item.destination,
                                   "Source and destination refer to the same filesystem item.");
            }
            if (item.collision && request.kind == OperationKind::Rename)
                return problem(ErrorCode::AlreadyExists, item.destination,
                               "Rename does not overwrite another item.");
            if (item.collision && request.collision == CollisionPolicy::Ask)
                return problem(ErrorCode::AlreadyExists, item.destination,
                               "Destination exists. Choose Skip all, Replace files, or Cancel before any "
                               "item is changed.");
            if (item.collision && request.collision == CollisionPolicy::Replace &&
                (item.directory || QFileInfo(item.destination).isDir() || isReparse(item.destination)))
                return problem(ErrorCode::Unsupported, item.destination,
                               "Directory merging and link replacement are not supported; use Skip or a new "
                               "destination.");
        }
        if (auto error = appendNode(item, source, {}, total))
            return *error;
        if (item.directory) {
            std::error_code ec;
            fs::recursive_directory_iterator iterator(nativePath(source), fs::directory_options::none, ec),
                end;
            if (ec)
                return translate(ec, source);
            while (iterator != end) {
                if (cancel->load())
                    return cancelled(source);
                const auto child = textPath(iterator->path());
                if (auto error = appendNode(item, child, QDir(source).relativeFilePath(child), total))
                    return *error;
                iterator.increment(ec);
                if (ec)
                    return translate(ec, source);
            }
        }
        items.push_back(std::move(item));
    }
    return items;
}
std::optional<Error> renameNoReplace(const QString &from, const QString &to) {
    const auto present = exists(to);
    if (const auto *error = std::get_if<Error>(&present))
        return *error;
    if (std::get<bool>(present))
        return problem(ErrorCode::AlreadyExists, to, "Destination became occupied; nothing was replaced.");
#ifdef Q_OS_WIN
    if (!MoveFileExW(nativePath(from).c_str(), nativePath(to).c_str(), 0))
        return translate(std::error_code(int(GetLastError()), std::system_category()), from, "Rename item");
#elif defined(Q_OS_LINUX)
    if (syscall(SYS_renameat2, AT_FDCWD, nativePath(from).c_str(), AT_FDCWD, nativePath(to).c_str(), 1) !=
        0) {
        const auto failure = errno;
        if (failure != EINVAL && failure != ENOSYS && failure != EOPNOTSUPP)
            return translate(std::error_code(failure, std::generic_category()), from, "Rename item");
        // WSL DrvFS can reject RENAME_NOREPLACE. Qt's documented rename contract
        // refuses an occupied destination; use it only for this filesystem limitation.
        if (QFileInfo(from).isDir()) {
            if (!QDir().rename(from, to))
                return problem(ErrorCode::Io, from, "Directory rename failed on this filesystem.");
        } else {
            QFile file(from);
            if (!file.rename(to))
                return problem(ErrorCode::Io, from, file.errorString());
        }
    }
#elif defined(Q_OS_MACOS)
    if (renamex_np(nativePath(from).c_str(), nativePath(to).c_str(), RENAME_EXCL) != 0)
        return translate(std::error_code(errno, std::generic_category()), from, "Rename item");
#else
    return problem(ErrorCode::Unsupported, from, "Atomic no-replace rename is unsupported on this platform.");
#endif
    return {};
}
std::optional<Error> removePath(const QString &path) {
    std::error_code ec;
    fs::remove_all(nativePath(path), ec);
    if (ec)
        return translate(ec, path, "Remove item");
    return {};
}
QString sibling(const QString &path, const QString &tag) {
    return QDir(QFileInfo(path).absolutePath())
        .filePath(".explorer-a-" + tag + "-" + QUuid::createUuid().toString(QUuid::WithoutBraces));
}
std::optional<Error> copyFile(const Node &node, const QString &destination, OperationProgress &state,
                              const ProgressCallback &progress, const CancelToken &cancel) {
    QFile input(node.source), output(destination);
    if (!input.open(QIODevice::ReadOnly))
        return problem(ErrorCode::Io, node.source, input.errorString());
    if (!output.open(QIODevice::WriteOnly | QIODevice::NewOnly))
        return problem(ErrorCode::Io, destination, output.errorString());
    QByteArray buffer(128 * 1024, Qt::Uninitialized);
    qint64 copied = 0;
    for (;;) {
        if (cancel->load())
            return cancelled(node.source);
        const auto count = input.read(buffer.data(), buffer.size());
        if (count < 0)
            return problem(ErrorCode::Io, node.source, input.errorString());
        if (!count)
            break;
        qint64 written = 0;
        while (written < count) {
            const auto amount = output.write(buffer.constData() + written, count - written);
            if (amount <= 0)
                return problem(ErrorCode::Io, destination, output.errorString());
            written += amount;
        }
        copied += count;
        state.bytes += count;
        state.path = node.source;
        progress(state);
    }
    if (!output.flush())
        return problem(ErrorCode::Io, destination, output.errorString());
    const QFileInfo after(node.source);
    if (copied != node.size || after.size() != node.size || after.lastModified() != node.modified)
        return problem(ErrorCode::Conflict, node.source,
                       "Source changed during copying; the original is preserved.");
    if (node.modified.isValid() && !output.setFileTime(node.modified, QFileDevice::FileModificationTime))
        return problem(ErrorCode::Io, destination,
                       "Could not preserve the source modification time: " + output.errorString());
    return {};
}
std::optional<Error> copyToStage(const Prepared &item, const QString &stage, OperationProgress &state,
                                 const ProgressCallback &progress, const CancelToken &cancel) {
    for (const auto &node : item.nodes) {
        if (cancel->load())
            return cancelled(node.source);
        const auto target = node.relative.isEmpty() ? stage : QDir(stage).filePath(node.relative);
        if (node.directory) {
            std::error_code ec;
            if (!fs::create_directory(nativePath(target), ec))
                return ec ? translate(ec, target, "Create staging directory")
                          : problem(ErrorCode::AlreadyExists, target, "Staging name unexpectedly exists.");
        } else if (auto error = copyFile(node, target, state, progress, cancel))
            return error;
    }
    return {};
}
bool unchanged(const Prepared &item) {
    for (const auto &node : item.nodes) {
        const QFileInfo info(node.source);
        if (!info.exists() || isReparse(node.source) || info.isDir() != node.directory ||
            (!node.directory && (info.size() != node.size || info.lastModified() != node.modified)))
            return false;
    }
    if (item.directory) {
        std::error_code ec;
        qsizetype count = 1;
        fs::recursive_directory_iterator iterator(nativePath(item.source), ec), end;
        if (ec)
            return false;
        while (iterator != end) {
            ++count;
            if (isReparse(textPath(iterator->path())))
                return false;
            iterator.increment(ec);
            if (ec)
                return false;
        }
        if (count != item.nodes.size())
            return false;
    }
    return true;
}
void cleanup(const QString &owned, ItemOutcome &outcome) {
    const auto result = retryFilesystemCleanup(owned);
    if (const auto *error = std::get_if<Error>(&result)) {
        outcome.retainedPaths.push_back(owned);
        outcome.cleanupPaths.push_back(owned);
        if (!outcome.error)
            outcome.error = *error;
    }
}
ItemOutcome transfer(const Prepared &item, const OperationRequest &request, OperationProgress &state,
                     const ProgressCallback &progress, const CancelToken &cancel) {
    ItemOutcome result{item.source, item.destination, false, false, {}, {}, {}};
    if (item.collision && request.collision == CollisionPolicy::Skip) {
        result.skipped = true;
        return result;
    }
    if (request.kind == OperationKind::Move && !item.collision) {
        if (cancel->load()) {
            result.error = cancelled(item.source);
            return result;
        }
        if (!renameNoReplace(item.source, item.destination)) {
            result.succeeded = true;
            for (const auto &node : item.nodes)
                state.bytes += node.size;
            return result;
        }
        if (!unchanged(item)) {
            result.error = problem(ErrorCode::Conflict, item.source,
                                   "Source changed before the move; it was preserved.");
            return result;
        }
    }
    const auto workspace = sibling(item.destination, "work");
    std::error_code allocationError;
    if (!fs::create_directory(nativePath(workspace), allocationError)) {
        result.error = allocationError
                           ? translate(allocationError, workspace, "Reserve operation workspace")
                           : problem(ErrorCode::AlreadyExists, workspace, "Workspace name is occupied.");
        return result;
    }
    if (auto error = rememberOwned(workspace)) {
        result.error = error;
        result.retainedPaths.push_back(workspace);
        return result;
    }
    const auto stage = QDir(workspace).filePath("payload");
    state.phase = "Copying";
    progress(state);
    if (auto error = copyToStage(item, stage, state, progress, cancel)) {
        result.error = error;
        cleanup(workspace, result);
        return result;
    }
    if (cancel->load()) {
        result.error = cancelled(item.source);
        cleanup(workspace, result);
        return result;
    }
    state.phase = "Committing";
    progress(state);
    QString backup;
    const auto occupied = exists(item.destination);
    if (const auto *error = std::get_if<Error>(&occupied)) {
        result.error = *error;
        cleanup(workspace, result);
        return result;
    }
    if (std::get<bool>(occupied)) {
        if (request.collision != CollisionPolicy::Replace || item.directory ||
            QFileInfo(item.destination).isDir() || isReparse(item.destination)) {
            result.error = problem(ErrorCode::AlreadyExists, item.destination,
                                   "Destination became occupied; no item was overwritten.");
            cleanup(workspace, result);
            return result;
        }
        backup = QDir(workspace).filePath("previous");
        if (auto error = renameNoReplace(item.destination, backup)) {
            result.error = error;
            cleanup(workspace, result);
            return result;
        }
    }
    if (auto error = renameNoReplace(stage, item.destination)) {
        result.error = error;
        if (!backup.isEmpty())
            if (auto restore = renameNoReplace(backup, item.destination)) {
                result.retainedPaths.push_back(workspace);
                result.cleanupPaths.push_back(workspace);
                result.error->detail +=
                    " Original destination backup retained: " + backup + ". " + restore->detail;
                return result;
            }
        cleanup(workspace, result);
        return result;
    }
    result.succeeded = true;
    state.phase = "Final cleanup";
    progress(state);
    cleanup(workspace, result);
    if (request.kind == OperationKind::Move) {
        if (cancel->load() || !unchanged(item)) {
            result.error = problem(ErrorCode::Conflict, item.source,
                                   "Destination copy completed; source was preserved because cancellation or "
                                   "a source change occurred before removal.");
            result.retainedPaths.push_back(item.source);
        } else if (auto error = removePath(item.source)) {
            result.error = error;
            result.retainedPaths.push_back(item.source);
        }
    }
    return result;
}
} // namespace
Result<Success> retryFilesystemCleanup(const QString &path) {
    QMutexLocker lock(&cleanupMutex);
    const auto owner = ownedDirectories.find(path);
    if (owner == ownedDirectories.end())
        return problem(ErrorCode::AccessDenied, path,
                       "No owned workspace record exists for this path; cleanup refused.");
    if (!owner.value()->matches(path))
        return problem(
            ErrorCode::Conflict, path,
            "Workspace identity is unresolved or changed; cleanup refused and ownership retained.");
    if (auto error = removePath(path))
        return *error;
    ownedDirectories.erase(owner);
    return Success{};
}
OperationResult performFilesystemOperation(const OperationRequest &request, const ProgressCallback &callback,
                                           const CancelToken &cancel) {
    const ProgressCallback progress = callback ? callback : [](const OperationProgress &) {};
    if (request.kind == OperationKind::Cleanup) {
        if (!request.confirmedDestructive)
            return problem(ErrorCode::AccessDenied, {},
                           "Cleanup of retained data needs explicit confirmation.");
        OperationReport report;
        for (const auto &path : request.sources) {
            ItemOutcome item;
            item.source = path;
            const auto result = retryFilesystemCleanup(path);
            if (const auto *error = std::get_if<Error>(&result)) {
                item.error = *error;
                item.retainedPaths.push_back(path);
                item.cleanupPaths.push_back(path);
            } else
                item.succeeded = true;
            report.items.push_back(std::move(item));
        }
        return report;
    }
    qint64 total = 0;
    progress({"Preflight", {}, 0, 0, 0, int(request.sources.size())});
    const auto planned = prepare(request, total, cancel);
    if (const auto *error = std::get_if<Error>(&planned))
        return *error;
    OperationReport report;
    const auto &items = std::get<QVector<Prepared>>(planned);
    OperationProgress state{"Working", {}, 0, total, 0, int(items.size())};
    for (const auto &item : items) {
        if (cancel->load()) {
            report.cancelled = true;
            break;
        }
        state.path = item.source;
        progress(state);
        ItemOutcome outcome{item.source, item.destination, false, false, {}, {}, {}};
        if (cancel->load()) {
            report.cancelled = true;
            break;
        }
        if (request.kind != OperationKind::NewFolder && !unchanged(item)) {
            outcome.error = problem(ErrorCode::Conflict, item.source,
                                    "Source changed after preflight; no mutation was started for this item.");
            report.items.push_back(std::move(outcome));
            ++state.items;
            continue;
        }
        if (request.kind == OperationKind::Copy || request.kind == OperationKind::Move)
            outcome = transfer(item, request, state, progress, cancel);
        else if (request.kind == OperationKind::Rename) {
            outcome.error = renameNoReplace(item.source, item.destination);
            outcome.succeeded = !outcome.error;
        } else if (request.kind == OperationKind::NewFolder) {
            std::error_code ec;
            outcome.succeeded = fs::create_directory(nativePath(item.destination), ec);
            if (!outcome.succeeded)
                outcome.error =
                    ec ? translate(ec, item.destination, "Create folder")
                       : problem(ErrorCode::AlreadyExists, item.destination, "Folder already exists.");
        } else if (request.kind == OperationKind::Trash) {
            QString trashPath;
            if (QFile::moveToTrash(item.source, &trashPath)) {
                outcome.succeeded = true;
                outcome.destination = trashPath;
            } else
                outcome.error = problem(
                    ErrorCode::Io, item.source,
                    "The system recycle operation failed. No permanent-delete fallback was attempted.");
        } else {
            outcome.error = removePath(item.source);
            outcome.succeeded = !outcome.error;
        }
        if (outcome.error && outcome.error->code == ErrorCode::Cancelled)
            report.cancelled = true;
        report.items.push_back(std::move(outcome));
        ++state.items;
        progress(state);
        if (report.cancelled)
            break;
    }
    state.phase = "Complete";
    progress(state);
    return report;
}
} // namespace explorer
