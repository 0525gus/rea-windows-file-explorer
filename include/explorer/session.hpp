#pragma once
#include "explorer/platform.hpp"
#include <QObject>
#include <QThreadPool>
#include <atomic>
#include <memory>
#include <stop_token>
namespace explorer {
struct TabState {
    quint64 id = 0, generation = 0;
    Location location;
    History history;
    QVector<Entry> entries, visible;
    QSet<QString> selection;
    QString query, pendingInput;
    SortSpec sort;
    std::optional<Error> error;
    bool loading = false;
    std::shared_ptr<std::stop_source> stop;
    std::shared_ptr<std::atomic<quint64>> examined;
};
class ExplorerSession : public QObject {
    Q_OBJECT
  public:
    explicit ExplorerSession(std::shared_ptr<IPlatform> platform, QObject *parent = nullptr);
    ~ExplorerSession() override;
    quint64 createTab(std::optional<Location> initial = {});
    void closeTab(quint64 id);
    const QVector<TabState> &tabs() const {
        return tabs_;
    }
    const TabState *tab(quint64 id) const;
    void navigate(quint64 id, const QString &input);
    void navigate(quint64 id, const Location &location);
    void back(quint64 id);
    void forward(quint64 id);
    void up(quint64 id);
    void refresh(quint64 id);
    void setSearch(quint64 id, const QString &query);
    void setSort(quint64 id, SortSpec sort);
    void setSelection(quint64 id, QSet<QString> keys);
    void openEntry(quint64 id, const QString &key);
    void shutdown();
    bool idle() const {
        return active_ == 0;
    }
    std::shared_ptr<IPlatform> platform() const {
        return platform_;
    }
  signals:
    void tabChanged(quint64 id);
    void tabsChanged();
    void shutdownFinished();

  private:
    TabState *mutableTab(quint64 id);
    void request(quint64 id, std::variant<QString, Location> input, int historyCursor, bool refresh,
                 bool parent = false);
    void derive(TabState &tab);
    std::shared_ptr<IPlatform> platform_;
    QThreadPool pool_;
    QVector<TabState> tabs_;
    quint64 nextId_ = 1;
    int active_ = 0;
    bool closing_ = false;
};
} // namespace explorer
