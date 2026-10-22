#pragma once
#include "explorer/session.hpp"
#include <QAbstractTableModel>
#include <QHash>
#include <QCache>
#include <QPixmap>
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
class QListView;
class QStackedWidget;
class QAbstractItemView;
class QMimeData;
class QFileSystemWatcher;
class QPlainTextEdit;
namespace explorer {
class EntryModel : public QAbstractTableModel {
    Q_OBJECT
  public:
    explicit EntryModel(QObject *parent = nullptr);
    ~EntryModel() override;
    std::function<QImage(const Entry &, int)> thumbnailProvider;
    void setThumbnailsEnabled(bool enabled);
    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : static_cast<int>(entries_.size());
    }
    int columnCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : 4;
    }
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    std::function<bool(const Entry &, const QString &)> renameEntry;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void setEntries(QVector<Entry> entries);
    const Entry *entry(int row) const;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

  private:
    QVector<Entry> entries_;
    void requestThumbnail(const Entry &entry);
    QThreadPool thumbnailPool_;
    QCache<QString, QPixmap> thumbnails_{16384};
    QSet<QString> thumbnailPending_;
    bool thumbnailsEnabled_ = true;
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
    qint64 lastRenderMilliseconds() const {
        return lastRenderMilliseconds_;
    }
    void setViewMode(int mode);

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
    void copySelection(bool cut);
    void pasteSelection();
    void renameSelection();
    void createDirectory();
    void recycleSelection();
    bool startOperation(OperationRequest request);
    bool acceptTransfer(const QMimeData *data, Qt::DropAction action, int row, bool internal);
    bool resolveTransfer(QStringList paths, Location destination, OperationKind kind);
    void renderOperation();
    void showFileMenu(QAbstractItemView *view, const QPoint &position);
    void toggleFavorite();
    void rebuildFavorites();
    void saveSettings();
    void syncWatches();
    void requestPreview(const Entry *entry);
    void loadPreview();
    void showNativeMenu(QVector<Entry> entries, Location location, QPoint position);
    void replayOperation(bool redo);
    QVector<Entry> selectedEntries() const;
    QAbstractItemView *activeView() const;
    QToolButton *makeButton(const QString &symbol, const QString &label, QWidget *parent);
    std::shared_ptr<IPlatform> platform_;
    ExplorerSession *session_;
    QTabBar *tabs_;
    QLineEdit *address_;
    QLineEdit *search_;
    QTreeWidget *tree_;
    QTableView *table_;
    QListView *icons_;
    QStackedWidget *views_;
    EntryModel *model_;
    QLabel *status_;
    QLabel *detailName_;
    QLabel *detailMeta_;
    QLabel *detailImage_;
    QLabel *error_;
    QProgressBar *progress_;
    QToolButton *back_;
    QToolButton *forward_;
    QToolButton *up_;
    QToolButton *newFolder_, *cut_, *copy_, *paste_, *rename_, *recycle_;
    QToolButton *cancelOperation_, *retryCleanup_;
    QToolButton *keepBoth_;
    QLabel *operationLabel_;
    QProgressBar *operationProgress_;
    QVector<Entry> clipboardEntries_;
    QString clipboardToken_;
    bool clipboardCut_ = false;
    int viewMode_ = 0;
    QHash<quint64, int> viewModes_;
    qint64 lastRenderMilliseconds_ = 0;
    QWidget *details_;
    QSplitter *splitter_;
    QTimer progressTimer_;
    QThreadPool treePool_;
    QThreadPool previewPool_;
    QFileSystemWatcher *fileWatcher_ = nullptr;
    QPlainTextEdit *previewText_;
    QTreeWidgetItem *favoritesRoot_;
    QStringList favorites_;
    QTimer watchTimer_, previewTimer_, searchTimer_;
    quint64 searchTab_ = 0, searchGeneration_ = 0;
    QString pendingSearch_;
    QSet<QString> changedPaths_;
    QString previewPath_;
    quint64 previewGeneration_ = 0;
    bool previewBusy_ = false;
    QHash<QString, std::shared_ptr<std::stop_source>> treeStops_;
    int treeActive_ = 0;
    bool closing_ = false, finalClose_ = false, updating_ = false;
};
} // namespace explorer
