#include "explorer/types.hpp"
#include <algorithm>
#include <QFileInfo>
#include <QRegularExpression>
#include <cmath>
namespace explorer {
QString operationName(OperationKind kind) {
    switch (kind) {
    case OperationKind::Undo:
        return QStringLiteral("Undo");
    case OperationKind::Redo:
        return QStringLiteral("Redo");
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
    // Explicit opt-in preserves literal searching for names containing spaces,
    // colons and other punctuation.
    if (!query.startsWith('?')) {
        const auto folded = query.toCaseFolded();
        QVector<Entry> result;
        for (const auto &entry : entries)
            if (entry.name.toCaseFolded().contains(folded))
                result.push_back(entry);
        return result;
    }
    static const QRegularExpression tokenizer(QStringLiteral(R"((?:[^\s"]|"[^"]*")+)"));
    QStringList terms;
    auto matches = tokenizer.globalMatch(query.mid(1));
    while (matches.hasNext()) {
        auto token = matches.next().captured();
        token.remove('"');
        terms.push_back(token);
    }
    auto sizeMatches = [](const Entry &entry, const QString &expression) {
        static const QRegularExpression format(
            QStringLiteral(R"(^(>=|<=|>|<|=)?(\d+(?:\.\d+)?)(b|kb|mb|gb|kib|mib|gib)?$)"),
            QRegularExpression::CaseInsensitiveOption);
        const auto match = format.match(expression);
        if (!match.hasMatch() || !entry.sizeBytes || entry.directory)
            return false;
        const auto unit = match.captured(3).toLower();
        const double multiplier = unit.startsWith('k') ? 1024.0 : unit.startsWith('m') ? 1048576.0
                                                  : unit.startsWith('g') ? 1073741824.0 : 1.0;
        const double threshold = match.captured(2).toDouble() * multiplier;
        if (!std::isfinite(threshold))
            return false;
        const auto size = static_cast<double>(*entry.sizeBytes);
        const auto op = match.captured(1);
        if (op == ">") return size > threshold;
        if (op == ">=") return size >= threshold;
        if (op == "<") return size < threshold;
        if (op == "<=") return size <= threshold;
        return size == threshold;
    };
    QVector<Entry> result;
    for (const auto &entry : entries) {
        bool accepted = true;
        for (const auto &term : terms) {
            const auto colon = term.indexOf(':');
            const auto field = colon < 0 ? QString() : term.left(colon).toLower();
            const auto value = colon < 0 ? term : term.mid(colon + 1);
            if (field == "ext") {
                auto extensions = value.toLower().split(',', Qt::SkipEmptyParts);
                for (auto &extension : extensions)
                    if (extension.startsWith('.')) extension.remove(0, 1);
                accepted = !entry.directory && extensions.contains(QFileInfo(entry.name).suffix().toLower());
            } else if (field == "type") {
                accepted = (value.compare("folder", Qt::CaseInsensitive) == 0 && entry.directory) ||
                           (value.compare("file", Qt::CaseInsensitive) == 0 && !entry.directory);
            } else if (field == "size") {
                accepted = sizeMatches(entry, value);
            } else if (field == "after" || field == "before") {
                const auto date = QDate::fromString(value, Qt::ISODate);
                const auto modified = entry.modifiedMs
                    ? QDateTime::fromMSecsSinceEpoch(*entry.modifiedMs).date() : QDate();
                accepted = date.isValid() && modified.isValid() &&
                    (field == "after" ? modified >= date : modified <= date);
            } else {
                accepted = entry.name.contains(field == "name" ? value : term, Qt::CaseInsensitive);
            }
            if (!accepted) break;
        }
        if (accepted) result.push_back(entry);
    }
    return result;
}

} // namespace explorer
