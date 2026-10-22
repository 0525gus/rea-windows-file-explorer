#pragma once
#include "browser_controller.h"
#include "file_model.h"
#include "file_views.h"
#include "operation_manager.h"
#include <QMainWindow>
class QLineEdit;
class QLabel;
class QTabBar;
class QTableView;
class QTreeWidget;
class QTreeWidgetItem;
class QToolButton;
class QProgressBar;
class QListView;
class QStackedWidget;

namespace explorer {
class MainWindow final : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(std::shared_ptr<Platform> platform = makeFilesystemPlatform(),
                        QWidget *parent = nullptr);
    BrowserController *controller() const {
        return controller_;
    }
    FileModel *fileModel() const {
        return model_;
    }
    FileProxy *proxy() const {
        return proxy_;
    }
    void openInitial(const Location &location);
    OperationManager *operations() const {
        return operations_;
    }

  private:
    void createUi();
    void synchronize(bool switchTab = false);
    void rebuildTabs();
    void updateStatus();
    void activateIndex(const QModelIndex &index);
    void addTreeLocation(QTreeWidgetItem *parent, const QString &title, const Location &location);
    void loadTreeItem(QTreeWidgetItem *item);
    void receiveTree(const Listing &listing, const QString &error);
    QAbstractItemView *currentView() const;
    void copySelection(bool cut);
    void paste();
    void newFolder();
    void renameSelected();
    void deleteSelected(bool permanent);
    void transferTo(bool move);
    void submit(OperationRequest request);
    void operationFinished(const OperationResult &);
    void showError(const QString &);
    void mutationProgress(const OperationProgress &);
    void setViewMode(int mode);
    BrowserController *controller_;
    FileModel *model_;
    FileProxy *proxy_;
    OperationManager *operations_;
    QTabBar *tabs_ = nullptr;
    QLineEdit *address_ = nullptr;
    QLineEdit *search_ = nullptr;
    QLabel *error_ = nullptr;
    QLabel *status_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QTableView *table_ = nullptr;
    QListView *list_ = nullptr;
    QStackedWidget *views_ = nullptr;
    FileViewInput *tableDrop_ = nullptr;
    FileViewInput *listDrop_ = nullptr;
    QVector<QToolButton *> mutationButtons_;
    QToolButton *viewButton_ = nullptr;
    QTreeWidget *tree_ = nullptr;
    QToolButton *back_ = nullptr;
    QToolButton *forward_ = nullptr;
    QToolButton *up_ = nullptr;
    QUuid renderedId_;
    quint64 renderedRevision_ = 0;
    bool synchronizing_ = false;
    OperationRequest lastRequest_;
    QString operationStatus_;
    QString operationError_;
};
} // namespace explorer
