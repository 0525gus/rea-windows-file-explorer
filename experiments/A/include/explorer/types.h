#pragma once
#include <QDateTime>
#include <QDir>
#include <QString>
#include <QVector>
#include <atomic>
#include <memory>
#include <variant>

namespace explorer {
struct Location {
    bool computer = true;
    QString path;
    static Location folder(QString path) {
        return {false, QDir::cleanPath(QDir(path).absolutePath())};
    }
    QString display() const {
        return computer ? QStringLiteral("This PC") : QDir::toNativeSeparators(path);
    }
    QString title() const {
        if (computer)
            return QStringLiteral("This PC");
        const auto name = QDir(path).dirName();
        return name.isEmpty() ? display() : name;
    }
    friend bool operator==(const Location &, const Location &) = default;
};
enum class ErrorCode {
    NotFound,
    AccessDenied,
    NotDirectory,
    AlreadyExists,
    InvalidName,
    Conflict,
    Unsupported,
    Cancelled,
    Io
};
struct Error {
    ErrorCode code = ErrorCode::Io;
    QString action;
    QString path;
    QString detail;
    int nativeCode = 0;
    QString message() const {
        return action + QStringLiteral(": ") + detail +
               (path.isEmpty() ? QString() : QStringLiteral("\n") + QDir::toNativeSeparators(path));
    }
};
template <class T> using Result = std::variant<T, Error>;
using Success = std::monostate;
using CancelToken = std::shared_ptr<std::atomic_bool>;
inline CancelToken cancellation() {
    return std::make_shared<std::atomic_bool>(false);
}
inline Error cancelled(const QString &path) {
    return {ErrorCode::Cancelled, QStringLiteral("Load folder"), path, QStringLiteral("Cancelled"), 0};
}
struct FileEntry {
    QString path;
    QString name;
    bool directory = false;
    bool link = false;
    qint64 size = -1;
    QDateTime modified;
    QString type;
};
struct Listing {
    Location location;
    QVector<FileEntry> entries;
    QStringList warnings;
};
struct Capabilities {
    bool browse = true;
    bool open = true;
    bool recycle = false;
    bool nativeProperties = false;
    bool fileOperations = false;
};
} // namespace explorer
