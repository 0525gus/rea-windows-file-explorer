#include "explorer/platform.hpp"
#include <QCollator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QProcess>
#include <QStandardPaths>
#include <cerrno>
#include <filesystem>
#include <sys/stat.h>

namespace explorer {
namespace {
namespace fs = std::filesystem;
constexpr quint64 listingBudget = 64ULL * 1024 * 1024;
Error ioError(const QString &operation, const QString &input, const std::error_code &error) {
    ErrorCode code = ErrorCode::Io;
    if (error == std::errc::no_such_file_or_directory)
        code = ErrorCode::NotFound;
    else if (error == std::errc::permission_denied || error == std::errc::operation_not_permitted)
        code = ErrorCode::PermissionDenied;
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
        return Location{Location::Kind::Directory, input, path, path, label};
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
            return Listing{location, std::move(entries), true};
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
        return Listing{location, std::move(entries), true};
    }
    int compareNames(const QString &a, const QString &b) const override {
        static thread_local QCollator collator = [] {
            QCollator result(QLocale::system());
            result.setNumericMode(true);
            result.setCaseSensitivity(Qt::CaseInsensitive);
            return result;
        }();
        return collator.compare(a, b);
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
#ifdef Q_OS_MACOS
        result.open = !QStandardPaths::findExecutable(QStringLiteral("open")).isEmpty();
#else
        result.open = !QStandardPaths::findExecutable(QStringLiteral("xdg-open")).isEmpty();
#endif
        return result;
    }
};
} // namespace
std::shared_ptr<IPlatform> makePlatform() {
    return std::make_shared<PosixPlatform>();
}
} // namespace explorer
