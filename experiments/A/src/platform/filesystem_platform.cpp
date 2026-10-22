#include "explorer/platform.h"
#include "filesystem_detail.h"
#include <QDesktopServices>
#include <QFileInfo>
#include <QStorageInfo>
#include <QUrl>
#include <filesystem>
#include <limits>
#include <system_error>
#ifdef Q_OS_WIN
#include <shellapi.h>
#endif

namespace explorer {
namespace {
using namespace filesystem_detail;
FileEntry entry(const fs::directory_entry &item, QStringList &warnings) {
    FileEntry result;
    result.path = textPath(item.path());
    result.name = textPath(item.path().filename());
    std::error_code ec;
    const auto linkStatus = item.symlink_status(ec);
    result.link = fs::is_symlink(linkStatus);
#ifdef Q_OS_WIN
    const auto attrs = GetFileAttributesW(item.path().c_str());
    result.link = result.link || (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT));
#endif
    result.directory = item.is_directory(ec);
    if (ec)
        warnings.push_back(result.name + QStringLiteral(": metadata unavailable"));
    if (!result.directory) {
        const auto size = item.file_size(ec);
        if (!ec && size <= quint64(std::numeric_limits<qint64>::max()))
            result.size = qint64(size);
    }
    result.modified = QFileInfo(result.path).lastModified();
    if (!result.modified.isValid())
        warnings.push_back(result.name + QStringLiteral(": modified time unavailable"));
    const auto suffix = QFileInfo(result.name).suffix();
    result.type = result.directory ? QStringLiteral("File folder")
                                   : (suffix.isEmpty() ? QStringLiteral("File")
                                                       : suffix.toUpper() + QStringLiteral(" file"));
    if (result.link)
        result.type += QStringLiteral(" (link)");
    return result;
}
class FilesystemPlatform final : public Platform {
  public:
    Capabilities capabilities() const override {
        return {true, true, true, false, true};
    }
    OperationResult mutate(const OperationRequest &request, const ProgressCallback &progress,
                           const CancelToken &cancel) const override {
        return performFilesystemOperation(request, progress, cancel);
    }
    Result<Success> retryCleanup(const QString &path) const override {
        return retryFilesystemCleanup(path);
    }
    Result<Listing> list(const Location &location, const CancelToken &cancel) const override {
        if (cancel->load())
            return cancelled(location.path);
        Listing result{location, {}, {}};
        if (location.computer) {
            for (const auto &drive : QDir::drives()) {
                result.entries.push_back({QDir::fromNativeSeparators(drive.absoluteFilePath()),
                                          drive.absoluteFilePath(),
                                          true,
                                          false,
                                          -1,
                                          {},
                                          QStringLiteral("Drive")});
            }
            return result;
        }
        std::error_code ec;
        const auto status = fs::status(nativePath(location.path), ec);
        if (ec)
            return translate(ec, location.path);
        if (!fs::is_directory(status))
            return Error{ErrorCode::NotDirectory, QStringLiteral("Open folder"), location.path,
                         QStringLiteral("The path is not a folder."), 0};
        fs::directory_iterator iterator(nativePath(location.path), ec);
        if (ec)
            return translate(ec, location.path);
        const fs::directory_iterator end;
        while (iterator != end) {
            if (cancel->load())
                return cancelled(location.path);
            result.entries.push_back(entry(*iterator, result.warnings));
            iterator.increment(ec);
            if (ec)
                return translate(ec, location.path);
        }
        return result;
    }
    Result<Success> openFile(const QString &path) const override {
        std::error_code ec;
        const bool exists = fs::exists(nativePath(path), ec);
        if (ec)
            return translate(ec, path);
        if (!exists)
            return Error{ErrorCode::NotFound, QStringLiteral("Open file"), path,
                         QStringLiteral("The file no longer exists."), 0};
#ifdef Q_OS_WIN
        const auto native = QDir::toNativeSeparators(path).toStdWString();
        const auto status = reinterpret_cast<INT_PTR>(
            ShellExecuteW(nullptr, L"open", native.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
        if (status <= 32)
            return Error{status == SE_ERR_ACCESSDENIED ? ErrorCode::AccessDenied : ErrorCode::Io,
                         QStringLiteral("Open file"), path,
                         QStringLiteral("Windows could not open the default application."), int(status)};
#else
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
            return Error{ErrorCode::Unsupported, QStringLiteral("Open file"), path,
                         QStringLiteral("No default application could be launched."), 0};
#endif
        return Success{};
    }
};
} // namespace
std::shared_ptr<Platform> makeFilesystemPlatform() {
    return std::make_shared<FilesystemPlatform>();
}
} // namespace explorer
