#pragma once
#include "navigation.h"
#include "platform.h"
#include <QFutureWatcher>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QThreadPool>
#include <QUuid>
#include <optional>

namespace explorer {
struct TabState {
    QUuid id;
    Location location;
    Listing listing;
    History history;
    QString query;
    int sortColumn = 0;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
    QStringList selection;
    int viewMode = 0;
    bool loading = false;
    std::optional<Error> error;
    quint64 generation = 0;
    quint64 revision = 0;
    CancelToken cancel = cancellation();
};
class BrowserController final : public QObject {
    Q_OBJECT
  public:
    explicit BrowserController(std::shared_ptr<Platform> platform, QObject *parent = nullptr);
    ~BrowserController() override;
    QUuid addTab(const Location &location = {});
    void closeTab(const QUuid &id);
    void setCurrent(const QUuid &id);
    QUuid currentId() const {
        return current_;
    }
    const TabState *current() const {
        return tab(current_);
    }
    const TabState *tab(const QUuid &id) const;
    const QVector<QUuid> &order() const {
        return order_;
    }
    void navigate(const Location &location);
    void back();
    void forward();
    void up();
    void refresh();
    void setQuery(const QString &query);
    void activate(const FileEntry &entry);
    void setSort(int column, Qt::SortOrder order);
    void setSelection(const QStringList &paths);
    void setViewMode(int mode);
    void refreshAll();
    void loadTree(const Location &location);
    Capabilities capabilities() const {
        return platform_->capabilities();
    }
  signals:
    void tabsChanged();
    void currentChanged();
    void tabChanged(const QUuid &id);
    void treeReady(const explorer::Listing &listing, const QString &error);

  private:
    void request(const QUuid &id, const Location &location, std::optional<int> cursor = {},
                 bool refresh = false);
    std::shared_ptr<Platform> platform_;
    QThreadPool pool_;
    QHash<QUuid, TabState> tabs_;
    QVector<QUuid> order_;
    QUuid current_;
    QSet<QFutureWatcher<Result<Listing>> *> watchers_;
    QVector<CancelToken> treeCancels_;
};
} // namespace explorer
