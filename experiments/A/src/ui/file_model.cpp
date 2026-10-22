#include "explorer/file_model.h"
#include <QApplication>
#include <QLocale>
#include <QMimeData>
#include <QStyle>
#include <QUrl>

namespace explorer {
QString formatBytes(qint64 bytes) {
    if (bytes < 0)
        return {};
    return QLocale().formattedDataSize(bytes, 1, QLocale::DataSizeTraditionalFormat);
}
FileModel::FileModel(QObject *parent) : QAbstractTableModel(parent) {}
Qt::ItemFlags FileModel::flags(const QModelIndex &index) const {
    return QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}
QStringList FileModel::mimeTypes() const {
    return {QStringLiteral("text/uri-list")};
}
QMimeData *FileModel::mimeData(const QModelIndexList &indexes) const {
    auto *mime = new QMimeData;
    QList<QUrl> urls;
    for (const auto &index : indexes)
        if (const auto *item = entry(index.row())) {
            const auto url = QUrl::fromLocalFile(item->path);
            if (!urls.contains(url))
                urls.push_back(url);
        }
    mime->setUrls(urls);
    return mime;
}
int FileModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : int(entries_.size());
}
int FileModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : Columns;
}
const FileEntry *FileModel::entry(int row) const {
    return row >= 0 && row < entries_.size() ? &entries_[row] : nullptr;
}
void FileModel::setEntries(const QVector<FileEntry> &entries) {
    beginResetModel();
    entries_ = entries;
    endResetModel();
}
QVariant FileModel::data(const QModelIndex &index, int role) const {
    const auto *item = entry(index.row());
    if (!item || !index.isValid())
        return {};
    if (role == Qt::ToolTipRole)
        return QDir::toNativeSeparators(item->path) +
               (item->link ? QStringLiteral("\nSymbolic link / reparse point") : QString());
    if (role == Qt::DecorationRole && index.column() == Name)
        return QApplication::style()->standardIcon(item->directory ? QStyle::SP_DirIcon
                                                                   : QStyle::SP_FileIcon);
    if (role == Qt::TextAlignmentRole && index.column() == Size)
        return int(Qt::AlignRight | Qt::AlignVCenter);
    if (role != Qt::DisplayRole)
        return {};
    switch (index.column()) {
    case Name:
        return item->name;
    case Modified:
        return item->modified.isValid()
                   ? QLocale().toString(item->modified, QStringLiteral("yyyy-MM-dd HH:mm"))
                   : QString();
    case Type:
        return item->type;
    case Size:
        return formatBytes(item->size);
    default:
        return {};
    }
}
QVariant FileModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case Name:
        return QStringLiteral("Name");
    case Modified:
        return QStringLiteral("Date modified");
    case Type:
        return QStringLiteral("Type");
    case Size:
        return QStringLiteral("Size");
    default:
        return {};
    }
}
FileProxy::FileProxy(QObject *parent) : QSortFilterProxyModel(parent) {
    collator_.setNumericMode(true);
    collator_.setCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(true);
}
void FileProxy::setQuery(const QString &query) {
    if (query_ != query) {
        query_ = query;
        invalidateFilter();
    }
}
bool FileProxy::filterAcceptsRow(int row, const QModelIndex &) const {
    const auto *model = qobject_cast<FileModel *>(sourceModel());
    const auto *item = model ? model->entry(row) : nullptr;
    return item && item->name.contains(query_, Qt::CaseInsensitive);
}
bool FileProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    const auto *model = qobject_cast<FileModel *>(sourceModel());
    const auto *a = model->entry(left.row());
    const auto *b = model->entry(right.row());
    if (a->directory != b->directory)
        return sortOrder() == Qt::AscendingOrder ? a->directory : !a->directory;
    if (left.column() == FileModel::Size && a->size != b->size)
        return a->size < b->size;
    if (left.column() == FileModel::Modified && a->modified != b->modified)
        return a->modified < b->modified;
    if (left.column() == FileModel::Type && a->type != b->type)
        return collator_.compare(a->type, b->type) < 0;
    const auto compared = collator_.compare(a->name, b->name);
    return compared == 0 ? a->path < b->path : compared < 0;
}
} // namespace explorer
