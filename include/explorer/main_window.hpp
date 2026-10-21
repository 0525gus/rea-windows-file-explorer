#pragma once
#include "explorer/session.hpp"
#include <QAbstractTableModel>
#include <QHash>
#include <QJsonObject>
#include <QMainWindow>
#include <QTimer>
#include <memory>
class QTabBar;
class QToolButton;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;
class QTableView;
class QLabel;
class QProgressBar;
class QSplitter;
namespace explorer {
class EntryModel : public QAbstractTableModel {
    Q_OBJECT
  public:
    explicit EntryModel(QObject *parent = nullptr) : QAbstractTableModel(parent) {}
    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : static_cast<int>(entries_.size());
    }
    int columnCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : 4;
    }
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void setEntries(QVector<Entry> entries);
    const Entry *entry(int row) const;

  private:
    QVector<Entry> entries_;
};
class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(std::shared_ptr<IPlatform> platform, QWidget *parent = nullptr);
    ~MainWindow() override;
    ExplorerSession *session() const {
        return session_;
    }
    quint64 currentTabId() const;
    void openPath(const QString &path);
    QJsonObject snapshot() const;

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    void syncTabs();
    void render(quint64 id);
    void navigateAddress();
    void updateSelection();
    void activateRow(const QModelIndex &index);
    void requestTree(QTreeWidgetItem *item);
    void finishClose();
    void showSortMenu();
    void showViewsMenu();
    QToolButton *makeButton(const QString &symbol, const QString &label, QWidget *parent);
    std::shared_ptr<IPlatform> platform_;
    ExplorerSession *session_;
    QTabBar *tabs_;
    QLineEdit *address_;
    QLineEdit *search_;
    QTreeWidget *tree_;
    QTableView *table_;
    EntryModel *model_;
    QLabel *status_;
    QLabel *detailName_;
    QLabel *detailMeta_;
    QLabel *error_;
    QProgressBar *progress_;
    QToolButton *back_;
    QToolButton *forward_;
    QToolButton *up_;
    QWidget *details_;
    QSplitter *splitter_;
    QTimer progressTimer_;
    QThreadPool treePool_;
    QHash<QString, std::shared_ptr<std::stop_source>> treeStops_;
    QHash<QString, QTreeWidgetItem *> treeItems_;
    int treeActive_ = 0;
    bool closing_ = false, finalClose_ = false, updating_ = false;
};
} // namespace explorer
