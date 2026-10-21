#pragma once
#include <QDateTime>
#include <QMetaType>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>
#include <optional>
#include <variant>
namespace explorer {
enum class ErrorCode {
    InvalidPath,
    NotFound,
    PermissionDenied,
    Unsupported,
    Io,
    Cancelled,
    Changed,
    Conflict,
    ResourceLimit
};
struct Error {
    ErrorCode code = ErrorCode::Io;
    QString operation, input, message;
    qint64 nativeCode = 0;
    QStringList partialPaths;
};
struct Unit {};
template <class T> using Result = std::variant<T, Error>;
template <class T> const Error *failure(const Result<T> &value) {
    return std::get_if<Error>(&value);
}
struct Location {
    enum class Kind { Drives, Directory };
    Kind kind = Kind::Drives;
    QString callerInput, path, key = QStringLiteral("drives"), label = QStringLiteral("This PC");
    bool operator==(const Location &other) const {
        return kind == other.kind && key == other.key;
    }
};
struct Entry {
    QString key, path, name, typeName, identity;
    bool directory = false;
    bool link = false;
    std::optional<quint64> sizeBytes;
    std::optional<qint64> modifiedMs;
};
struct Listing {
    Location location;
    QVector<Entry> entries;
    bool complete = true;
};
struct Progress {
    quint64 examined = 0;
    QString phase;
};
using ProgressSink = std::function<void(const Progress &)>;
struct Capabilities {
    bool open = false;
    bool copy = false;
    bool move = false;
    bool recycle = false;
    bool rename = false;
    bool mkdir = false;
};
enum class SortColumn { Name, Size, Modified };
struct SortSpec {
    SortColumn column = SortColumn::Name;
    bool ascending = true;
};
using NameComparer = std::function<int(const QString &, const QString &)>;
QString errorCodeName(ErrorCode code);
void sortEntries(QVector<Entry> &entries, SortSpec spec, const NameComparer &compare);
QVector<Entry> filterEntries(const QVector<Entry> &entries, const QString &query);
class History {
  public:
    const QVector<Location> &locations() const {
        return locations_;
    }
    int cursor() const {
        return cursor_;
    }
    bool canBack() const {
        return cursor_ > 0;
    }
    bool canForward() const {
        return cursor_ + 1 < locations_.size();
    }
    std::optional<Location> at(int index) const;
    void visit(const Location &location);
    bool commitCursor(int index);

  private:
    QVector<Location> locations_;
    int cursor_ = -1;
};
} // namespace explorer
Q_DECLARE_METATYPE(explorer::Error)
