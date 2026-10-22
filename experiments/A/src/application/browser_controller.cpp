#include "explorer/browser_controller.h"
#include <QtConcurrentRun>

namespace explorer {
BrowserController::BrowserController(std::shared_ptr<Platform> platform, QObject *parent)
    : QObject(parent), platform_(std::move(platform)) {
    pool_.setMaxThreadCount(2);
}
BrowserController::~BrowserController() {
    for (auto &state : tabs_)
        state.cancel->store(true);
    for (const auto &token : treeCancels_)
        token->store(true);
    for (auto *watcher : watchers_)
        disconnect(watcher, nullptr, this, nullptr);
    pool_.waitForDone();
}
const TabState *BrowserController::tab(const QUuid &id) const {
    const auto found = tabs_.constFind(id);
    return found == tabs_.cend() ? nullptr : &found.value();
}
QUuid BrowserController::addTab(const Location &location) {
    const auto id = QUuid::createUuid();
    TabState state;
    state.id = id;
    tabs_.insert(id, state);
    order_.push_back(id);
    current_ = id;
    emit tabsChanged();
    emit currentChanged();
    request(id, location);
    return id;
}
void BrowserController::closeTab(const QUuid &id) {
    auto found = tabs_.find(id);
    if (found == tabs_.end())
        return;
    found->cancel->store(true);
    const auto index = order_.indexOf(id);
    tabs_.erase(found);
    order_.removeAt(index);
    if (order_.isEmpty()) {
        addTab();
        return;
    }
    if (current_ == id)
        current_ = order_[qMin(index, order_.size() - 1)];
    emit tabsChanged();
    emit currentChanged();
}
void BrowserController::setCurrent(const QUuid &id) {
    if (!tabs_.contains(id) || current_ == id)
        return;
    current_ = id;
    emit currentChanged();
}
void BrowserController::navigate(const Location &location) {
    if (!current_.isNull())
        request(current_, location);
}
void BrowserController::back() {
    const auto *state = current();
    if (state && state->history.canBack())
        request(current_, *state->history.at(state->history.cursor() - 1), state->history.cursor() - 1);
}
void BrowserController::forward() {
    const auto *state = current();
    if (state && state->history.canForward())
        request(current_, *state->history.at(state->history.cursor() + 1), state->history.cursor() + 1);
}
void BrowserController::up() {
    if (const auto *state = current())
        navigate(parentOf(state->location));
}
void BrowserController::refresh() {
    if (const auto *state = current())
        request(current_, state->location, {}, true);
}
void BrowserController::setQuery(const QString &query) {
    if (auto found = tabs_.find(current_); found != tabs_.end()) {
        found->query = query;
        emit tabChanged(current_);
    }
}
void BrowserController::activate(const FileEntry &entry) {
    if (entry.directory) {
        navigate(Location::folder(entry.path));
        return;
    }
    const auto result = platform_->openFile(entry.path);
    auto found = tabs_.find(current_);
    if (found != tabs_.end()) {
        if (const auto *error = std::get_if<Error>(&result))
            found->error = *error;
        else
            found->error.reset();
        emit tabChanged(current_);
    }
}
void BrowserController::setSort(int column, Qt::SortOrder order) {
    if (auto state = tabs_.find(current_); state != tabs_.end()) {
        state->sortColumn = column;
        state->sortOrder = order;
    }
}
void BrowserController::setSelection(const QStringList &paths) {
    if (auto state = tabs_.find(current_); state != tabs_.end())
        state->selection = paths;
}
void BrowserController::setViewMode(int mode) {
    if (auto state = tabs_.find(current_); state != tabs_.end()) {
        state->viewMode = mode;
        emit tabChanged(current_);
    }
}
void BrowserController::refreshAll() {
    const auto ids = order_;
    for (const auto &id : ids)
        if (const auto *state = tab(id))
            request(id, state->location, {}, true);
}
void BrowserController::loadTree(const Location &location) {
    const auto cancel = cancellation();
    treeCancels_.push_back(cancel);
    auto *watcher = new QFutureWatcher<Result<Listing>>(this);
    watchers_.insert(watcher);
    connect(watcher, &QFutureWatcher<Result<Listing>>::finished, this, [this, watcher, location, cancel] {
        const auto result = watcher->result();
        watchers_.remove(watcher);
        treeCancels_.removeAll(cancel);
        watcher->deleteLater();
        if (const auto *listing = std::get_if<Listing>(&result))
            emit treeReady(*listing, {});
        else
            emit treeReady(Listing{location, {}, {}}, std::get<Error>(result).message());
    });
    const auto platform = platform_;
    watcher->setFuture(
        QtConcurrent::run(&pool_, [platform, location, cancel] { return platform->list(location, cancel); }));
}
void BrowserController::request(const QUuid &id, const Location &location, std::optional<int> cursor,
                                bool refresh) {
    auto found = tabs_.find(id);
    if (found == tabs_.end())
        return;
    found->cancel->store(true);
    found->cancel = cancellation();
    const auto cancel = found->cancel;
    const auto generation = ++found->generation;
    found->loading = true;
    found->error.reset();
    emit tabChanged(id);
    auto *watcher = new QFutureWatcher<Result<Listing>>(this);
    watchers_.insert(watcher);
    connect(watcher, &QFutureWatcher<Result<Listing>>::finished, this,
            [this, watcher, id, generation, cursor, refresh] {
                const auto result = watcher->result();
                watchers_.remove(watcher);
                watcher->deleteLater();
                auto state = tabs_.find(id);
                if (state == tabs_.end() || state->generation != generation)
                    return;
                state->loading = false;
                if (const auto *listing = std::get_if<Listing>(&result)) {
                    state->listing = *listing;
                    ++state->revision;
                    state->location = listing->location;
                    if (!refresh) {
                        state->query.clear();
                        state->selection.clear();
                    } else {
                        QStringList retained;
                        for (const auto &entry : listing->entries)
                            if (state->selection.contains(entry.path))
                                retained.push_back(entry.path);
                        state->selection = retained;
                    }
                    if (cursor)
                        state->history.commitCursor(*cursor);
                    else if (!refresh)
                        state->history.visit(listing->location);
                } else if (const auto *error = std::get_if<Error>(&result);
                           error->code != ErrorCode::Cancelled)
                    state->error = *error;
                emit tabChanged(id);
                emit tabsChanged();
            });
    const auto platform = platform_;
    watcher->setFuture(QtConcurrent::run(&pool_, [platform, location, cancel]() -> Result<Listing> {
        try {
            return platform->list(location, cancel);
        } catch (const std::exception &e) {
            return Error{ErrorCode::Io, QStringLiteral("Open folder"), location.path,
                         QString::fromLocal8Bit(e.what()), 0};
        }
    }));
}
} // namespace explorer
