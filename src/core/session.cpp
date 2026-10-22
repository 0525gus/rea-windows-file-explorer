#include "explorer/session.hpp"
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QHash>
#include <QTimer>
#include <QtConcurrentRun>
#include <algorithm>
namespace explorer {
ExplorerSession::ExplorerSession(std::shared_ptr<IPlatform> platform, QObject *parent)
    : QObject(parent), platform_(std::move(platform)) {
    pool_.setMaxThreadCount(std::clamp(QThread::idealThreadCount(), 1, 4));
}
ExplorerSession::~ExplorerSession() {
    if (operationStop_)
        operationStop_->request_stop();
    for (auto &state : tabs_)
        if (state.stop)
            state.stop->request_stop();
    pool_.waitForDone();
}
const TabState *ExplorerSession::tab(quint64 id) const {
    for (const auto &state : tabs_)
        if (state.id == id)
            return &state;
    return nullptr;
}
TabState *ExplorerSession::mutableTab(quint64 id) {
    for (auto &state : tabs_)
        if (state.id == id)
            return &state;
    return nullptr;
}
quint64 ExplorerSession::createTab(std::optional<Location> initial) {
    if (closing_)
        return 0;
    TabState state;
    state.id = nextId_++;
    const auto id = state.id;
    tabs_.push_back(std::move(state));
    emit tabsChanged();
    navigate(id, initial.value_or(Location{}));
    return id;
}
void ExplorerSession::closeTab(quint64 id) {
    if (auto *state = mutableTab(id); state && state->stop)
        state->stop->request_stop();
    tabs_.erase(std::remove_if(tabs_.begin(), tabs_.end(), [=](const auto &state) { return state.id == id; }),
                tabs_.end());
    emit tabsChanged();
}
void ExplorerSession::derive(TabState &state) {
    QElapsedTimer elapsed;
    elapsed.start();
    state.visible = filterEntries(state.entries, state.query);
    const auto adapter = platform_;
    sortEntries(state.visible, state.sort,
                [adapter](const QString &a, const QString &b) { return adapter->compareNames(a, b); });
    QSet<QString> visibleKeys;
    for (const auto &entry : state.visible)
        visibleKeys.insert(entry.key);
    state.selection.intersect(visibleKeys);
    state.deriveMilliseconds = elapsed.elapsed();
}
void ExplorerSession::request(quint64 id, std::variant<QString, Location> input, int historyCursor,
                              bool isRefresh, bool parent) {
    auto *state = mutableTab(id);
    if (!state || closing_)
        return;
    if (active_ >= 12) {
        state->error = Error{ErrorCode::ResourceLimit,
                             QStringLiteral("browse"),
                             {},
                             QStringLiteral("Too many pending requests. Try again when loading finishes.")};
        emit tabChanged(id);
        return;
    }
    if (state->stop)
        state->stop->request_stop();
    const quint64 generation = ++state->generation;
    state->stop = std::make_shared<std::stop_source>();
    state->examined = std::make_shared<std::atomic<quint64>>(0);
    state->loading = true;
    state->error.reset();
    state->pendingInput = std::holds_alternative<QString>(input) ? std::get<QString>(input)
                                                                 : std::get<Location>(input).callerInput;
    const auto stop = state->stop->get_token();
    const auto progress = state->examined;
    const auto adapter = platform_;
    ++active_;
    emit tabChanged(id);
    auto *watcher = new QFutureWatcher<Result<Listing>>(this);
    connect(watcher, &QFutureWatcher<Result<Listing>>::finished, this,
            [this, watcher, id, generation, historyCursor, isRefresh] {
                const auto result = watcher->result();
                watcher->deleteLater();
                --active_;
                if (auto *current = mutableTab(id);
                    current && current->generation == generation && !closing_) {
                    current->loading = false;
                    if (const auto *error = failure(result))
                        current->error = *error;
                    else {
                        const auto &listing = std::get<Listing>(result);
                        if (listing.complete) {
                            QHash<QString, QString> oldIdentities;
                            for (const auto &entry : current->entries)
                                oldIdentities.insert(entry.key, entry.identity);
                            QSet<QString> kept;
                            for (const auto &entry : listing.entries)
                                if (current->selection.contains(entry.key) && !entry.identity.isEmpty() &&
                                    oldIdentities.value(entry.key) == entry.identity)
                                    kept.insert(entry.key);
                            current->selection = std::move(kept);
                            if (!(current->location == listing.location))
                                current->query.clear();
                            current->location = listing.location;
                            current->entries = listing.entries;
                            if (historyCursor >= 0)
                                current->history.commitCursor(historyCursor);
                            else if (!isRefresh)
                                current->history.visit(listing.location);
                            current->error.reset();
                            current->pendingInput.clear();
                            derive(*current);
                        } else
                            current->error =
                                Error{ErrorCode::Io, QStringLiteral("browse"), listing.location.callerInput,
                                      QStringLiteral("Incomplete directory listing was not committed.")};
                    }
                    emit tabChanged(id);
                }
                if (closing_ && active_ == 0)
                    emit shutdownFinished();
            });
    watcher->setFuture(QtConcurrent::run(
        &pool_, [adapter, input = std::move(input), stop, progress, parent]() -> Result<Listing> {
            Result<Location> parsed = std::holds_alternative<QString>(input)
                                          ? adapter->parseLocation(std::get<QString>(input))
                                          : Result<Location>(std::get<Location>(input));
            if (const auto *error = failure(parsed))
                return *error;
            if (parent) {
                auto result = adapter->parent(std::get<Location>(parsed));
                if (const auto *error = failure(result))
                    return *error;
                const auto &location = std::get<std::optional<Location>>(result);
                if (location)
                    parsed = *location;
            }
            if (stop.stop_requested())
                return Error{ErrorCode::Cancelled,
                             QStringLiteral("browse"),
                             {},
                             QStringLiteral("Request superseded.")};
            return adapter->list(std::get<Location>(parsed), stop,
                                 [progress](const Progress &value) { progress->store(value.examined); });
        }));
}
void ExplorerSession::navigate(quint64 id, const QString &input) {
    request(id, input, -1, false);
}
void ExplorerSession::navigate(quint64 id, const Location &location) {
    request(id, location, -1, false);
}
void ExplorerSession::back(quint64 id) {
    if (const auto *state = tab(id); state && state->history.canBack()) {
        const int index = state->history.cursor() - 1;
        request(id, *state->history.at(index), index, false);
    }
}
void ExplorerSession::forward(quint64 id) {
    if (const auto *state = tab(id); state && state->history.canForward()) {
        const int index = state->history.cursor() + 1;
        request(id, *state->history.at(index), index, false);
    }
}
void ExplorerSession::up(quint64 id) {
    const auto *state = tab(id);
    if (state && state->location.kind != Location::Kind::Drives)
        request(id, state->location, -1, false, true);
}
void ExplorerSession::refresh(quint64 id) {
    if (const auto *state = tab(id))
        request(id, state->location, -1, true);
}
void ExplorerSession::setSearch(quint64 id, const QString &query) {
    if (auto *state = mutableTab(id)) {
        state->query = query;
        derive(*state);
        emit tabChanged(id);
    }
}
void ExplorerSession::setSort(quint64 id, SortSpec sort) {
    if (auto *state = mutableTab(id)) {
        state->sort = sort;
        derive(*state);
        emit tabChanged(id);
    }
}
void ExplorerSession::setSelection(quint64 id, QSet<QString> keys) {
    if (auto *state = mutableTab(id)) {
        state->selection = std::move(keys);
    }
}
void ExplorerSession::openEntry(quint64 id, const QString &key) {
    const auto *state = tab(id);
    if (!state)
        return;
    const auto found = std::find_if(state->visible.begin(), state->visible.end(),
                                    [&](const auto &entry) { return entry.key == key; });
    if (found == state->visible.end())
        return;
    if (found->directory) {
        navigate(id, found->path);
        return;
    }
    if (closing_)
        return;
    if (active_ >= 12) {
        mutableTab(id)->error =
            Error{ErrorCode::ResourceLimit, QStringLiteral("open"), found->path,
                  QStringLiteral("Too many pending requests. Try again when loading finishes.")};
        emit tabChanged(id);
        return;
    }
    const auto generation = state->generation;
    const Entry entry = *found;
    const auto adapter = platform_;
    const auto stop = state->stop ? state->stop->get_token() : std::stop_token{};
    ++active_;
    auto *watcher = new QFutureWatcher<Result<Unit>>(this);
    connect(watcher, &QFutureWatcher<Result<Unit>>::finished, this, [this, watcher, id, generation] {
        const auto result = watcher->result();
        watcher->deleteLater();
        --active_;
        if (auto *current = mutableTab(id); current && current->generation == generation && !closing_) {
            if (const auto *error = failure(result))
                current->error = *error;
            else
                current->error.reset();
            emit tabChanged(id);
        }
        if (closing_ && active_ == 0)
            emit shutdownFinished();
    });
    watcher->setFuture(QtConcurrent::run(&pool_, [adapter, entry, stop]() -> Result<Unit> {
        if (stop.stop_requested())
            return Error{ErrorCode::Cancelled, QStringLiteral("open"), entry.path,
                         QStringLiteral("Open request cancelled before launch.")};
        return adapter->open(entry);
    }));
}
void ExplorerSession::shutdown() {
    if (closing_)
        return;
    closing_ = true;
    if (operationStop_)
        operationStop_->request_stop();
    for (auto &state : tabs_)
        if (state.stop)
            state.stop->request_stop();
    if (active_ == 0)
        QTimer::singleShot(0, this, &ExplorerSession::shutdownFinished);
}
OperationState ExplorerSession::operationState() const {
    auto state = operation_;
    if (operationMonitor_) {
        std::lock_guard lock(operationMonitor_->mutex);
        state.progress = operationMonitor_->progress;
    }
    return state;
}
bool ExplorerSession::beginOperation(OperationRequest request) {
    if (closing_ || operation_.running || (operation_.report && !operation_.report->cleanupPending.isEmpty()))
        return false;
    operation_ = OperationState{};
    operation_.running = true;
    operation_.request = request;
    operationMonitor_ = std::make_shared<OperationMonitor>();
    operationStop_ = std::make_shared<std::stop_source>();
    const auto monitor = operationMonitor_;
    const auto stop = operationStop_->get_token();
    const auto adapter = platform_;
    ++active_;
    emit operationChanged();
    auto *watcher = new QFutureWatcher<OperationReport>(this);
    connect(watcher, &QFutureWatcher<OperationReport>::finished, this, [this, watcher] {
        operation_.report = watcher->result();
        watcher->deleteLater();
        --active_;
        operation_.running = false;
        emit operationChanged();
        if (!closing_) {
            QVector<quint64> ids;
            for (const auto &state : tabs_)
                ids.push_back(state.id);
            for (auto id : ids)
                refresh(id);
        }
        if (closing_ && active_ == 0)
            emit shutdownFinished();
    });
    watcher->setFuture(QtConcurrent::run(&pool_, [adapter, request = std::move(request), monitor, stop] {
        return adapter->execute(request, stop, [monitor](const OperationProgress &progress) {
            std::lock_guard lock(monitor->mutex);
            monitor->progress = progress;
        });
    }));
    return true;
}
void ExplorerSession::cancelOperation() {
    if (operationStop_)
        operationStop_->request_stop();
}
void ExplorerSession::retryOperationCleanup(const QString &path) {
    if (closing_ || operation_.running || !operation_.report ||
        !operation_.report->cleanupPending.contains(path))
        return;
    operation_.running = true;
    ++active_;
    emit operationChanged();
    const auto adapter = platform_;
    auto *watcher = new QFutureWatcher<Result<Unit>>(this);
    connect(watcher, &QFutureWatcher<Result<Unit>>::finished, this, [this, watcher, path] {
        const auto result = watcher->result();
        watcher->deleteLater();
        --active_;
        operation_.running = false;
        if (!failure(result))
            operation_.report->cleanupPending.removeAll(path);
        else
            operation_.report->items.push_back({{}, path, ItemStatus::Partial, *failure(result), {path}});
        emit operationChanged();
        if (closing_ && active_ == 0)
            emit shutdownFinished();
    });
    watcher->setFuture(QtConcurrent::run(&pool_, [adapter, path] { return adapter->retryCleanup(path); }));
}
} // namespace explorer
