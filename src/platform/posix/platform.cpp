#include "explorer/platform.hpp"
#include "../common/file_operations.hpp"
#include <QCollator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QProcess>
#include <QStandardPaths>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>
#ifdef Q_OS_LINUX
#include <linux/fs.h>
#include <sys/syscall.h>
#endif

namespace explorer {
namespace {
using platform_detail::NativeOperations;
using platform_detail::Node;
namespace fs = std::filesystem;
constexpr quint64 listingBudget = 64ULL * 1024 * 1024;
Error ioError(const QString &operation, const QString &input, const std::error_code &error) {
    ErrorCode code = ErrorCode::Io;
    if (error == std::errc::no_such_file_or_directory)
        code = ErrorCode::NotFound;
    else if (error == std::errc::permission_denied || error == std::errc::operation_not_permitted)
        code = ErrorCode::PermissionDenied;
    else if (error == std::errc::file_exists)
        code = ErrorCode::Conflict;
    else if (error == std::errc::not_a_directory || error == std::errc::invalid_argument)
        code = ErrorCode::InvalidPath;
    else if (error == std::errc::filename_too_long || error == std::errc::operation_not_supported)
        code = ErrorCode::Unsupported;
    return {code, operation, input, QString::fromStdString(error.message()), error.value(), {}};
}
fs::path nativePath(const QString &path) {
    return fs::path(QFile::encodeName(path).constData());
}
QString fromNative(const fs::path &path) {
    return QFile::decodeName(path.native().c_str());
}
QString fileIdentity(const struct stat &info) {
    return QString::number(static_cast<qulonglong>(info.st_dev), 16) + QLatin1Char(':') +
           QString::number(static_cast<qulonglong>(info.st_ino), 16);
}
Node operationNodeValue(const struct stat &info) {
#ifdef Q_OS_MACOS
    const qint64 timestamp =
        static_cast<qint64>(info.st_mtimespec.tv_sec) * 1000000000 + info.st_mtimespec.tv_nsec;
#else
    const qint64 timestamp = static_cast<qint64>(info.st_mtim.tv_sec) * 1000000000 + info.st_mtim.tv_nsec;
#endif
    return {fileIdentity(info), S_ISDIR(info.st_mode), S_ISLNK(info.st_mode),
            info.st_size >= 0 ? static_cast<quint64>(info.st_size) : 0, timestamp};
}
Result<Node> operationNode(const QString &path) {
    struct stat info{};
    const QByteArray native = QFile::encodeName(path);
    if (::lstat(native.constData(), &info) != 0)
        return ioError(QStringLiteral("inspect operation object"), path,
                       std::error_code(errno, std::generic_category()));
    if (!S_ISREG(info.st_mode) && !S_ISDIR(info.st_mode) && !S_ISLNK(info.st_mode))
        return Error{ErrorCode::Unsupported,
                     QStringLiteral("inspect operation object"),
                     path,
                     QStringLiteral("Special filesystem objects are unsupported"),
                     0,
                     {}};
    return operationNodeValue(info);
}
struct FileDescriptor {
    int value = -1;
    ~FileDescriptor() {
        if (value >= 0)
            ::close(value);
    }
};
NativeOperations operationHooks() {
    NativeOperations hooks;
    hooks.inspect = operationNode;
    hooks.createExclusive = [](const QString &path, bool directory) -> Result<Node> {
        const QByteArray native = QFile::encodeName(path);
        if (directory) {
            if (::mkdir(native.constData(), 0700) != 0)
                return ioError(QStringLiteral("create directory"), path,
                               std::error_code(errno, std::generic_category()));
            auto node = operationNode(path);
            if (const auto *error = failure(node)) {
                Error retained = *error;
                retained.partialPaths << path;
                return retained;
            }
            return node;
        }
        FileDescriptor descriptor{
            ::open(native.constData(), O_CREAT | O_EXCL | O_NOFOLLOW | O_WRONLY | O_CLOEXEC, 0600)};
        if (descriptor.value < 0)
            return ioError(QStringLiteral("create staging file"), path,
                           std::error_code(errno, std::generic_category()));
        struct stat info{};
        if (::fstat(descriptor.value, &info) != 0) {
            Error error = ioError(QStringLiteral("inspect staging file"), path,
                                  std::error_code(errno, std::generic_category()));
            error.partialPaths << path;
            return error;
        }
        return operationNodeValue(info);
    };
    hooks.moveNoReplace = [](const QString &from, const Node &expected, const QString &to,
                             const Node &destinationExpected) -> Result<bool> {
        auto inspected = operationNode(from);
        if (const auto *error = failure(inspected))
            return *error;
        const Node node = std::get<Node>(inspected);
        if (expected.identity.isEmpty() || node.identity != expected.identity || node.link ||
            node.directory != expected.directory ||
            (!node.directory && (node.size != expected.size || node.modified != expected.modified)))
            return Error{ErrorCode::Changed,
                         QStringLiteral("move without replacement"),
                         from,
                         QStringLiteral("Rename source identity or contents changed"),
                         0,
                         {}};
        const QByteArray parentPath = QFile::encodeName(QFileInfo(to).absolutePath());
        FileDescriptor destinationParent{
            ::open(parentPath.constData(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC)};
        if (destinationParent.value < 0)
            return ioError(QStringLiteral("open rename destination"), to,
                           std::error_code(errno, std::generic_category()));
        struct stat destinationInfo{};
        if (::fstat(destinationParent.value, &destinationInfo) != 0)
            return ioError(QStringLiteral("inspect rename destination"), to,
                           std::error_code(errno, std::generic_category()));
        if (fileIdentity(destinationInfo) != destinationExpected.identity ||
            !S_ISDIR(destinationInfo.st_mode))
            return Error{ErrorCode::Changed,
                         QStringLiteral("move without replacement"),
                         to,
                         QStringLiteral("Destination directory identity changed"),
                         0,
                         {}};
        const QByteArray source = QFile::encodeName(from),
                         targetLeaf = QFile::encodeName(QFileInfo(to).fileName());
#ifdef Q_OS_LINUX
        const int result =
            static_cast<int>(::syscall(SYS_renameat2, AT_FDCWD, source.constData(), destinationParent.value,
                                       targetLeaf.constData(), RENAME_NOREPLACE));
#elif defined(Q_OS_MACOS)
        const int result = ::renameatx_np(AT_FDCWD, source.constData(), destinationParent.value,
                                          targetLeaf.constData(), RENAME_EXCL);
#else
        return Error{ErrorCode::Unsupported,
                     QStringLiteral("move without replacement"),
                     to,
                     QStringLiteral("Atomic no-replace rename is unavailable"),
                     0,
                     {}};
#endif
        if (result == 0)
            return true;
        const int code = errno;
        if (code == EXDEV)
            return false;
        if (code == ENOSYS || code == EINVAL || code == EOPNOTSUPP) {
#ifdef Q_OS_LINUX
            if (!expected.directory) {
                // linkat publishes a complete regular file without replacing
                // an existing name. /proc/self/fd binds the source to this
                // verified descriptor, rather than re-resolving a user link.
                FileDescriptor input{::open(source.constData(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC)};
                if (input.value < 0)
                    return ioError(QStringLiteral("open no-replace link source"), from,
                                   std::error_code(errno, std::generic_category()));
                struct stat sourceInfo{};
                if (::fstat(input.value, &sourceInfo) != 0)
                    return ioError(QStringLiteral("inspect no-replace link source"), from,
                                   std::error_code(errno, std::generic_category()));
                const Node sourceNode = operationNodeValue(sourceInfo);
                if (!S_ISREG(sourceInfo.st_mode) || sourceNode.identity != expected.identity ||
                    sourceNode.size != expected.size || sourceNode.modified != expected.modified)
                    return Error{ErrorCode::Changed,
                                 QStringLiteral("publish no-replace link"),
                                 from,
                                 QStringLiteral("Source changed before hard-link publication"),
                                 0,
                                 {}};
                const QByteArray descriptorPath =
                    QByteArrayLiteral("/proc/self/fd/") + QByteArray::number(input.value);
                if (::linkat(AT_FDCWD, descriptorPath.constData(), destinationParent.value,
                             targetLeaf.constData(), AT_SYMLINK_FOLLOW) != 0) {
                    const int linkCode = errno;
                    if (linkCode == EXDEV)
                        return false;
                    if (linkCode == EINVAL || linkCode == ENOSYS || linkCode == EOPNOTSUPP)
                        return Error{ErrorCode::Unsupported,
                                     QStringLiteral("publish no-replace link"),
                                     to,
                                     QStringLiteral("Filesystem supports neither no-replace rename nor "
                                                    "descriptor-bound hard-link publication"),
                                     linkCode,
                                     {}};
                    return ioError(QStringLiteral("publish no-replace link"), to,
                                   std::error_code(linkCode, std::generic_category()));
                }
                auto partial = [&](Error error) -> Result<bool> {
                    error.partialPaths << from << to;
                    return error;
                };
                struct stat targetInfo{};
                if (::fstatat(destinationParent.value, targetLeaf.constData(), &targetInfo,
                              AT_SYMLINK_NOFOLLOW) != 0)
                    return partial(ioError(QStringLiteral("verify published link"), to,
                                           std::error_code(errno, std::generic_category())));
                if (!S_ISREG(targetInfo.st_mode) || fileIdentity(targetInfo) != expected.identity)
                    return partial({ErrorCode::Changed,
                                    QStringLiteral("verify published link"),
                                    to,
                                    QStringLiteral("Published target identity changed"),
                                    0,
                                    {}});
                auto current = operationNode(from);
                if (const auto *error = failure(current)) {
                    if (error->code == ErrorCode::NotFound)
                        return true;
                    return partial(*error);
                }
                const Node latest = std::get<Node>(current);
                if (latest.identity != expected.identity || latest.link || latest.size != expected.size ||
                    latest.modified != expected.modified)
                    return partial(
                        {ErrorCode::Changed,
                         QStringLiteral("finish no-replace link move"),
                         from,
                         QStringLiteral("Source changed after target publication; source was preserved"),
                         0,
                         {}});
                if (::unlink(source.constData()) != 0) {
                    if (errno == ENOENT)
                        return true;
                    return partial(ioError(QStringLiteral("finish no-replace link move"), from,
                                           std::error_code(errno, std::generic_category())));
                }
                return true;
            }
#endif
            return Error{ErrorCode::Unsupported,
                         QStringLiteral("move without replacement"),
                         to,
                         QStringLiteral("This filesystem does not support atomic no-replace directory rename "
                                        "(native error %1); use a supported native filesystem")
                             .arg(code),
                         code,
                         {}};
        }
        return ioError(QStringLiteral("move without replacement"), to,
                       std::error_code(code, std::generic_category()));
    };
    hooks.remove = [](const QString &path, const Node &expected, bool contents) -> Result<Unit> {
        auto inspected = operationNode(path);
        if (const auto *error = failure(inspected))
            return *error;
        const auto &node = std::get<Node>(inspected);
        if (expected.identity.isEmpty() || node.identity != expected.identity ||
            node.directory != expected.directory || node.link ||
            (contents && !node.directory &&
             (node.size != expected.size || node.modified != expected.modified)))
            return Error{ErrorCode::Changed,
                         QStringLiteral("owned cleanup"),
                         path,
                         QStringLiteral("Cleanup object identity changed"),
                         0,
                         {}};
        const QByteArray native = QFile::encodeName(path);
        if ((node.directory ? ::rmdir(native.constData()) : ::unlink(native.constData())) != 0)
            return ioError(QStringLiteral("owned cleanup"), path,
                           std::error_code(errno, std::generic_category()));
        return Unit{};
    };
    hooks.copyContents = [](const QString &source, const Node &expected, const QString &target,
                            const Node &targetExpected, std::stop_token stop,
                            const std::function<void(quint64)> &progress) -> Result<Unit> {
        const QByteArray sourceNative = QFile::encodeName(source), targetNative = QFile::encodeName(target);
        FileDescriptor input{::open(sourceNative.constData(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC)};
        if (input.value < 0)
            return ioError(QStringLiteral("read copy source"), source,
                           std::error_code(errno, std::generic_category()));
        struct stat sourceInfo{};
        if (::fstat(input.value, &sourceInfo) != 0)
            return ioError(QStringLiteral("inspect open source"), source,
                           std::error_code(errno, std::generic_category()));
        const Node before = operationNodeValue(sourceInfo);
        if (!S_ISREG(sourceInfo.st_mode) || before.identity != expected.identity ||
            before.size != expected.size || before.modified != expected.modified)
            return Error{ErrorCode::Changed,
                         QStringLiteral("copy"),
                         source,
                         QStringLiteral("Source changed before reading"),
                         0,
                         {}};
        FileDescriptor output{::open(targetNative.constData(), O_WRONLY | O_NOFOLLOW | O_CLOEXEC)};
        if (output.value < 0)
            return ioError(QStringLiteral("write staging copy"), target,
                           std::error_code(errno, std::generic_category()));
        struct stat targetInfo{};
        if (::fstat(output.value, &targetInfo) != 0)
            return ioError(QStringLiteral("inspect staging copy"), target,
                           std::error_code(errno, std::generic_category()));
        if (!S_ISREG(targetInfo.st_mode) || fileIdentity(targetInfo) != targetExpected.identity)
            return Error{ErrorCode::Changed,
                         QStringLiteral("copy"),
                         target,
                         QStringLiteral("Staging object changed before writing"),
                         0,
                         {}};
        QByteArray buffer(1024 * 1024, Qt::Uninitialized);
        quint64 copied = 0;
        while (true) {
            if (stop.stop_requested())
                return Error{ErrorCode::Cancelled,
                             QStringLiteral("copy"),
                             source,
                             QStringLiteral("Copy cancelled"),
                             0,
                             {}};
            const ssize_t read = ::read(input.value, buffer.data(), static_cast<size_t>(buffer.size()));
            if (read < 0) {
                if (errno == EINTR)
                    continue;
                return ioError(QStringLiteral("read copy source"), source,
                               std::error_code(errno, std::generic_category()));
            }
            if (!read)
                break;
            ssize_t offset = 0;
            while (offset < read) {
                const ssize_t written =
                    ::write(output.value, buffer.constData() + offset, static_cast<size_t>(read - offset));
                if (written < 0 && errno == EINTR)
                    continue;
                if (written <= 0)
                    return ioError(QStringLiteral("write staging copy"), target,
                                   std::error_code(written == 0 ? EIO : errno, std::generic_category()));
                offset += written;
            }
            copied += static_cast<quint64>(read);
            progress(static_cast<quint64>(read));
        }
        struct stat afterInfo{};
        if (::fstat(input.value, &afterInfo) != 0)
            return ioError(QStringLiteral("verify copy source"), source,
                           std::error_code(errno, std::generic_category()));
        const Node after = operationNodeValue(afterInfo);
        if (copied != expected.size || after.identity != expected.identity || after.size != expected.size ||
            after.modified != expected.modified)
            return Error{ErrorCode::Changed,
                         QStringLiteral("copy"),
                         source,
                         QStringLiteral("Source changed during copying"),
                         0,
                         {}};
#ifdef Q_OS_MACOS
        const timespec times[] = {sourceInfo.st_atimespec, sourceInfo.st_mtimespec};
#else
        const timespec times[] = {sourceInfo.st_atim, sourceInfo.st_mtim};
#endif
        if (::fchmod(output.value, sourceInfo.st_mode & 0777) != 0 || ::futimens(output.value, times) != 0 ||
            ::fsync(output.value) != 0)
            return ioError(QStringLiteral("finish staging copy"), target,
                           std::error_code(errno, std::generic_category()));
        return Unit{};
    };
    hooks.recycle = [](const QString &path, const Node &expected) -> Result<Unit> {
        auto current = operationNode(path);
        if (const auto *error = failure(current))
            return *error;
        if (std::get<Node>(current).identity != expected.identity || std::get<Node>(current).link)
            return Error{ErrorCode::Changed,
                         QStringLiteral("trash"),
                         path,
                         QStringLiteral("Source identity changed"),
                         0,
                         {}};
        QFile file(path);
        if (!file.moveToTrash())
            return Error{ErrorCode::Io,      QStringLiteral("trash"),           path,
                         file.errorString(), static_cast<qint64>(file.error()), {}};
        return Unit{};
    };
    hooks.trashWithReceipt = [](const QString &path, const Node &expected) -> Result<platform_detail::TrashReceipt> {
        auto current = operationNode(path);
        if (const auto *error = failure(current)) return *error;
        if (std::get<Node>(current).identity != expected.identity || std::get<Node>(current).link)
            return Error{ErrorCode::Changed, "Trash", path, "Source identity changed"};
        QString trashed;
        if (!QFile::moveToTrash(path, &trashed))
            return Error{ErrorCode::Io, "Trash", path, "Could not move item to OS Trash"};
        const QFileInfo trashInfo(trashed);
        const auto metadata = QDir(trashInfo.absolutePath()).dirName() == "files"
            ? QDir(trashInfo.absolutePath()).absoluteFilePath("../info/" + trashInfo.fileName() + ".trashinfo")
            : QString();
        return platform_detail::TrashReceipt{trashed, trashed, metadata};
    };
    hooks.restoreTrash = [move = hooks.moveNoReplace](const platform_detail::TrashReceipt &receipt,
                                                     const QString &target) -> Result<QString> {
        auto source = operationNode(receipt.path), parent = operationNode(QFileInfo(target).absolutePath());
        if (const auto *error = failure(source)) return *error;
        if (const auto *error = failure(parent)) return *error;
        auto result = move(receipt.path, std::get<Node>(source), target, std::get<Node>(parent));
        if (const auto *error = failure(result)) return *error;
        if (!std::get<bool>(result))
            return Error{ErrorCode::Unsupported, "Restore", target, "Trash restore crosses filesystems"};
        if (!receipt.metadata.isEmpty()) QFile::remove(receipt.metadata);
        return target;
    };
    return hooks;
}
std::optional<qint64> modifiedMs(const struct stat &info) {
#ifdef Q_OS_MACOS
    return static_cast<qint64>(info.st_mtimespec.tv_sec) * 1000 + info.st_mtimespec.tv_nsec / 1000000;
#else
    return static_cast<qint64>(info.st_mtim.tv_sec) * 1000 + info.st_mtim.tv_nsec / 1000000;
#endif
}
Error withPartial(Error error, const QVector<Entry> &entries) {
    for (const auto &entry : entries)
        error.partialPaths.push_back(entry.path);
    return error;
}
class PosixPlatform final : public IPlatform {
  public:
    Result<Location> parseLocation(const QString &input) const override {
        if (input.isEmpty() || input.contains(QChar(u'\0')))
            return Error{ErrorCode::InvalidPath,
                         QStringLiteral("parse"),
                         input,
                         QStringLiteral("Enter a filesystem directory path"),
                         0,
                         {}};
        const QString path = QDir::cleanPath(QDir::current().absoluteFilePath(input));
        std::error_code error;
        const auto status = fs::status(nativePath(path), error);
        if (error)
            return ioError(QStringLiteral("parse"), input, error);
        if (!fs::exists(status))
            return ioError(QStringLiteral("parse"), input,
                           std::make_error_code(std::errc::no_such_file_or_directory));
        if (!fs::is_directory(status))
            return ioError(QStringLiteral("parse"), input, std::make_error_code(std::errc::not_a_directory));
        QString label = QFileInfo(path).fileName();
        if (label.isEmpty())
            label = path;
        struct stat info{};
        const QByteArray lookup = QFile::encodeName(path);
        const QString objectIdentity =
            ::lstat(lookup.constData(), &info) == 0 ? fileIdentity(info) : QString{};
        return Location{Location::Kind::Directory, input, path, path, label, objectIdentity};
    }
    Result<std::optional<Location>> parent(const Location &location) const override {
        if (location.kind == Location::Kind::Drives)
            return std::optional<Location>{};
        if (location.path == QStringLiteral("/"))
            return std::optional<Location>{Location{}};
        const QString path = fromNative(nativePath(location.path).parent_path());
        if (!path.startsWith(QLatin1Char('/')))
            return Error{ErrorCode::InvalidPath,
                         QStringLiteral("parent"),
                         location.path,
                         QStringLiteral("Location is not an absolute filesystem path"),
                         0,
                         {}};
        QString label = QFileInfo(path).fileName();
        if (label.isEmpty())
            label = path;
        return std::optional<Location>{Location{Location::Kind::Directory, path, path, path, label}};
    }
    Result<Listing> list(const Location &location, std::stop_token stop,
                         ProgressSink progress) const override {
        Location currentLocation = location;
        if (currentLocation.kind == Location::Kind::Directory) {
            struct stat info{};
            const QByteArray lookup = QFile::encodeName(location.path);
            currentLocation.identity =
                ::lstat(lookup.constData(), &info) == 0 ? fileIdentity(info) : QString{};
        }
        QVector<Entry> entries;
        const auto cancelled = [&]() {
            return withPartial({ErrorCode::Cancelled,
                                QStringLiteral("list"),
                                location.callerInput,
                                QStringLiteral("Directory loading cancelled"),
                                0,
                                {}},
                               entries);
        };
        if (stop.stop_requested())
            return cancelled();
        if (location.kind == Location::Kind::Drives) {
            Entry root;
            root.key = root.path = root.name = QStringLiteral("/");
            root.directory = true;
            root.typeName = QStringLiteral("Filesystem");
            struct stat info{};
            if (::lstat("/", &info) == 0)
                root.identity = fileIdentity(info);
            entries.push_back(std::move(root));
            return Listing{currentLocation, std::move(entries), true};
        }
        std::error_code error;
        fs::directory_iterator iterator(nativePath(location.path), fs::directory_options::none, error), end;
        if (error)
            return ioError(QStringLiteral("list"), location.callerInput, error);
        quint64 bytes = 0;
        while (iterator != end) {
            if (stop.stop_requested())
                return cancelled();
            Entry entry;
            entry.path = fromNative(iterator->path());
            const std::string sourcePath = iterator->path().native();
            if (QFile::encodeName(entry.path) !=
                QByteArray(sourcePath.data(), static_cast<qsizetype>(sourcePath.size())))
                return withPartial(
                    {ErrorCode::Unsupported,
                     QStringLiteral("decode filename"),
                     location.path,
                     QStringLiteral(
                         "Filename bytes cannot round-trip through the current filesystem codec: %1")
                         .arg(QString::fromLatin1(
                             QByteArray(sourcePath.data(), static_cast<qsizetype>(sourcePath.size()))
                                 .toHex())),
                     0,
                     {}},
                    entries);
            entry.key = entry.path;
            entry.name = fromNative(iterator->path().filename());
            const auto status = iterator->symlink_status(error);
            if (error)
                return withPartial(ioError(QStringLiteral("item status"), entry.path, error), entries);
            entry.link = fs::is_symlink(status);
            entry.directory = fs::is_directory(status);
            if (entry.link) {
                // Explicit user navigation may follow a directory link; enumeration never recurses.
                std::error_code targetError;
                const auto target = iterator->status(targetError);
                if (!targetError)
                    entry.directory = fs::is_directory(target);
            }
            struct stat info{};
            const QByteArray native = QFile::encodeName(entry.path);
            if (::lstat(native.constData(), &info) == 0) {
                entry.identity = fileIdentity(info);
                entry.modifiedMs = modifiedMs(info);
                if (S_ISREG(info.st_mode) && info.st_size >= 0)
                    entry.sizeBytes = static_cast<quint64>(info.st_size);
            }
            entry.typeName = entry.link        ? QStringLiteral("Symbolic link")
                             : entry.directory ? QStringLiteral("Folder")
                                               : QStringLiteral("File");
            const quint64 addition =
                sizeof(Entry) +
                2ULL * static_cast<quint64>(entry.key.size() + entry.path.size() + entry.name.size() +
                                            entry.typeName.size() + entry.identity.size());
            if (addition > listingBudget - bytes)
                return withPartial({ErrorCode::ResourceLimit,
                                    QStringLiteral("list"),
                                    location.callerInput,
                                    QStringLiteral("Directory listing exceeds the 64 MiB estimated budget"),
                                    0,
                                    {}},
                                   entries);
            bytes += addition;
            entries.push_back(std::move(entry));
            if (progress)
                progress({static_cast<quint64>(entries.size()), QStringLiteral("Reading directory")});
            iterator.increment(error);
            if (error)
                return withPartial(ioError(QStringLiteral("next folder item"), location.callerInput, error),
                                   entries);
        }
        if (stop.stop_requested())
            return cancelled();
        return Listing{currentLocation, std::move(entries), true};
    }
    int compareNames(const QString &a, const QString &b) const override {
        static thread_local QCollator collator = [] {
            QCollator result(QLocale::system());
            // Some Qt6 POSIX/C-locale backends ignore numericMode. Numeric runs
            // are owned by this comparator rather than delegated to that backend.
            result.setNumericMode(false);
            result.setCaseSensitivity(Qt::CaseInsensitive);
            return result;
        }();
        const auto digit = [](QChar value) { return value >= QLatin1Char('0') && value <= QLatin1Char('9'); };
        qsizetype left = 0, right = 0;
        while (left < a.size() && right < b.size()) {
            if (digit(a[left]) && digit(b[right])) {
                qsizetype leftEnd = left, rightEnd = right;
                while (leftEnd < a.size() && digit(a[leftEnd]))
                    ++leftEnd;
                while (rightEnd < b.size() && digit(b[rightEnd]))
                    ++rightEnd;
                while (left < leftEnd && a[left] == QLatin1Char('0'))
                    ++left;
                while (right < rightEnd && b[right] == QLatin1Char('0'))
                    ++right;
                if (leftEnd - left != rightEnd - right)
                    return leftEnd - left < rightEnd - right ? -1 : 1;
                const int order = QString::compare(a.mid(left, leftEnd - left),
                                                   b.mid(right, rightEnd - right), Qt::CaseSensitive);
                if (order)
                    return order;
                left = leftEnd;
                right = rightEnd;
            } else {
                qsizetype leftEnd = left + 1, rightEnd = right + 1;
                if (!digit(a[left]) && !digit(b[right])) {
                    while (leftEnd < a.size() && !digit(a[leftEnd]))
                        ++leftEnd;
                    while (rightEnd < b.size() && !digit(b[rightEnd]))
                        ++rightEnd;
                }
                const int order =
                    collator.compare(a.mid(left, leftEnd - left), b.mid(right, rightEnd - right));
                if (order)
                    return order;
                left = leftEnd;
                right = rightEnd;
            }
        }
        return left == a.size() ? (right == b.size() ? 0 : -1) : 1;
    }
    Result<Entry> inspectEntry(const QString &input) const override {
        if (input.isEmpty() || input.contains(QChar(u'\0')))
            return Error{ErrorCode::InvalidPath,
                         QStringLiteral("inspect entry"),
                         input,
                         QStringLiteral("Enter a local filesystem path"),
                         0,
                         {}};
        Entry entry;
        entry.path = QDir::cleanPath(QDir::current().absoluteFilePath(input));
        auto inspected = operationNode(entry.path);
        if (const auto *error = failure(inspected))
            return *error;
        const Node node = std::get<Node>(inspected);
        entry.key = entry.path;
        entry.name = QFileInfo(entry.path).fileName();
        entry.identity = node.identity;
        entry.directory = node.directory;
        entry.link = node.link;
        entry.typeName = node.link        ? QStringLiteral("Symbolic link")
                         : node.directory ? QStringLiteral("Folder")
                                          : QStringLiteral("File");
        if (!node.directory && !node.link)
            entry.sizeBytes = node.size;
        entry.modifiedMs = node.modified / 1000000;
        return entry;
    }
    Result<Unit> open(const Entry &entry) const override {
        struct stat info{};
        const QByteArray native = QFile::encodeName(entry.path);
        if (::lstat(native.constData(), &info) != 0)
            return ioError(QStringLiteral("open"), entry.path,
                           std::error_code(errno, std::generic_category()));
        if (entry.identity.isEmpty() || fileIdentity(info) != entry.identity)
            return Error{ErrorCode::Changed,
                         QStringLiteral("open"),
                         entry.path,
                         QStringLiteral("File identity is unknown or changed; refresh before opening"),
                         0,
                         {}};
#ifdef Q_OS_MACOS
        const QString executable = QStandardPaths::findExecutable(QStringLiteral("open"));
#else
        const QString executable = QStandardPaths::findExecutable(QStringLiteral("xdg-open"));
#endif
        if (executable.isEmpty())
            return Error{ErrorCode::Unsupported,
                         QStringLiteral("open"),
                         entry.path,
                         QStringLiteral("OS default application helper is unavailable"),
                         0,
                         {}};
        if (!QProcess::startDetached(executable, QStringList{entry.path}))
            return Error{ErrorCode::Io,
                         QStringLiteral("open"),
                         entry.path,
                         QStringLiteral("Cannot launch the OS default application helper"),
                         0,
                         {}};
        return Unit{};
    }
    Capabilities capabilities() const override {
        Capabilities result;
        result.copy = result.move = result.recycle = result.rename = result.mkdir = true;
#ifdef Q_OS_MACOS
        result.open = !QStandardPaths::findExecutable(QStringLiteral("open")).isEmpty();
#else
        result.open = !QStandardPaths::findExecutable(QStringLiteral("xdg-open")).isEmpty();
#endif
        return result;
    }
    OperationReport execute(const OperationRequest &request, std::stop_token stop,
                            OperationProgressSink progress) const override {
        return operations_->execute(request, stop, std::move(progress));
    }
    OperationHistory operationHistory() const override { return operations_->history(); }
    OperationReport replayOperation(bool redo, std::stop_token stop, OperationProgressSink sink) const override {
        return operations_->replay(redo, stop, std::move(sink));
    }
    Result<Unit> retryCleanup(const QString &path) const override {
        return operations_->retryCleanup(path);
    }

  private:
    std::shared_ptr<platform_detail::FileOperations> operations_ =
        std::make_shared<platform_detail::FileOperations>(operationHooks());
};
} // namespace
std::shared_ptr<IPlatform> makePlatform() {
    return std::make_shared<PosixPlatform>();
}
} // namespace explorer
