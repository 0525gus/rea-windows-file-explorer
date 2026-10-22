#pragma once
#include "explorer/types.h"
#include <filesystem>
#include <system_error>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif
namespace explorer::filesystem_detail {
namespace fs = std::filesystem;
inline fs::path nativePath(const QString &path) {
#ifdef Q_OS_WIN
    return fs::path(path.toStdWString());
#else
    return fs::path(path.toUtf8().constData());
#endif
}
inline QString textPath(const fs::path &path) {
#ifdef Q_OS_WIN
    return QDir::fromNativeSeparators(QString::fromStdWString(path.native()));
#else
    return QString::fromUtf8(path.native().data(), qsizetype(path.native().size()));
#endif
}
inline Error translate(const std::error_code &ec, const QString &path,
                       const QString &action = QStringLiteral("Open folder")) {
    auto code = ErrorCode::Io;
    if (ec == std::errc::permission_denied)
        code = ErrorCode::AccessDenied;
    else if (ec == std::errc::no_such_file_or_directory)
        code = ErrorCode::NotFound;
    else if (ec == std::errc::not_a_directory)
        code = ErrorCode::NotDirectory;
    else if (ec == std::errc::file_exists)
        code = ErrorCode::AlreadyExists;
    return {code, action, path, QString::fromLocal8Bit(ec.message().c_str()), ec.value()};
}
inline bool isReparse(const QString &path) {
#ifdef Q_OS_WIN
    const auto attrs = GetFileAttributesW(nativePath(path).c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    std::error_code ec;
    return fs::is_symlink(fs::symlink_status(nativePath(path), ec));
#endif
}
} // namespace explorer::filesystem_detail
