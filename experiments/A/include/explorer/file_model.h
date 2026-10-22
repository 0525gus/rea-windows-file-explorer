#pragma once
#include "types.h"
#include <QAbstractTableModel>
#include <QCollator>
#include <QSortFilterProxyModel>

namespace explorer {
class FileModel final : public QAbstractTableModel {
    Q_OBJECT
  public:
    enum Column { Name, Modified, Type, Size, Columns };
    explicit FileModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &) const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &) const override;
    Qt::DropActions supportedDragActions() const override {
        return Qt::CopyAction | Qt::MoveAction;
    }
    void setEntries(const QVector<FileEntry> &entries);
    const FileEntry *entry(int row) const;
    const QVector<FileEntry> &entries() const {
        return entries_;
    }

  private:
    QVector<FileEntry> entries_;
};
class FileProxy final : public QSortFilterProxyModel {
    Q_OBJECT
  public:
    explicit FileProxy(QObject *parent = nullptr);
    void setQuery(const QString &query);

  protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

  private:
    QString query_;
    QCollator collator_;
};
QString formatBytes(qint64 bytes);
} // namespace explorer
