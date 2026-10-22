#include "explorer/types.hpp"
#include <algorithm>
namespace explorer {
QString operationName(OperationKind kind) {
    switch (kind) {
    case OperationKind::Copy:
        return QStringLiteral("Copy");
    case OperationKind::Move:
        return QStringLiteral("Move");
    case OperationKind::Recycle:
        return QStringLiteral("Move to Recycle Bin");
    case OperationKind::Rename:
        return QStringLiteral("Rename");
    case OperationKind::CreateDirectory:
        return QStringLiteral("New folder");
    }
    return QStringLiteral("File operation");
}
QString errorCodeName(ErrorCode code) {
    switch (code) {
    case ErrorCode::InvalidPath:
        return QStringLiteral("Invalid path");
    case ErrorCode::NotFound:
        return QStringLiteral("Not found");
    case ErrorCode::PermissionDenied:
        return QStringLiteral("Access denied");
    case ErrorCode::Unsupported:
        return QStringLiteral("Unsupported");
    case ErrorCode::Cancelled:
        return QStringLiteral("Cancelled");
    case ErrorCode::Changed:
        return QStringLiteral("Item changed");
    case ErrorCode::Conflict:
        return QStringLiteral("Name conflict");
    case ErrorCode::ResourceLimit:
        return QStringLiteral("Resource limit");
    case ErrorCode::Io:
        return QStringLiteral("File system error");
    }
    return QStringLiteral("Unknown error");
}
std::optional<Location> History::at(int index) const {
    if (index < 0 || index >= locations_.size())
        return std::nullopt;
    return locations_[index];
}
void History::visit(const Location &location) {
    if (cursor_ >= 0 && locations_[cursor_] == location)
        return;
    locations_.resize(cursor_ + 1);
    locations_.push_back(location);
    cursor_ = static_cast<int>(locations_.size()) - 1;
}
bool History::commitCursor(int index) {
    if (!at(index))
        return false;
    cursor_ = index;
    return true;
}
void sortEntries(QVector<Entry> &entries, SortSpec spec, const NameComparer &compare) {
    std::sort(entries.begin(), entries.end(), [&](const Entry &a, const Entry &b) {
        if (a.directory != b.directory)
            return a.directory;
        int order = 0;
        if (spec.column == SortColumn::Size) {
            if (a.sizeBytes.has_value() != b.sizeBytes.has_value())
                return a.sizeBytes.has_value();
            if (a.sizeBytes && b.sizeBytes)
                order = *a.sizeBytes < *b.sizeBytes ? -1 : (*a.sizeBytes > *b.sizeBytes ? 1 : 0);
        } else if (spec.column == SortColumn::Modified) {
            if (a.modifiedMs.has_value() != b.modifiedMs.has_value())
                return a.modifiedMs.has_value();
            if (a.modifiedMs && b.modifiedMs)
                order = *a.modifiedMs < *b.modifiedMs ? -1 : (*a.modifiedMs > *b.modifiedMs ? 1 : 0);
        } else
            order = compare(a.name, b.name);
        if (order != 0)
            return spec.ascending ? order < 0 : order > 0;
        order = compare(a.name, b.name);
        if (order == 0)
            order = QString::compare(a.name, b.name, Qt::CaseSensitive);
        if (order == 0)
            order = QString::compare(a.key, b.key, Qt::CaseSensitive);
        return order < 0;
    });
}
QVector<Entry> filterEntries(const QVector<Entry> &entries, const QString &query) {
    if (query.isEmpty())
        return entries;
    const auto folded = query.toCaseFolded();
    QVector<Entry> result;
    for (const auto &entry : entries)
        if (entry.name.toCaseFolded().contains(folded))
            result.push_back(entry);
    return result;
}
} // namespace explorer
