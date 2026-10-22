#include "explorer/operation_manager.h"
#include <QElapsedTimer>
#include <QtConcurrentRun>
namespace explorer {
OperationManager::OperationManager(std::shared_ptr<Platform> platform, QObject *parent)
    : QObject(parent), platform_(std::move(platform)) {
    pool_.setMaxThreadCount(1);
    connect(this, &OperationManager::progressProduced, this, &OperationManager::progressChanged,
            Qt::QueuedConnection);
}
OperationManager::~OperationManager() {
    cancel();
    if (watcher_)
        disconnect(watcher_, nullptr, this, nullptr);
    pool_.waitForDone();
}
void OperationManager::cancel() {
    cancel_->store(true);
}
bool OperationManager::start(const OperationRequest &request) {
    if (busy_)
        return false;
    cancel_ = cancellation();
    busy_ = true;
    watcher_ = new QFutureWatcher<OperationResult>(this);
    connect(watcher_, &QFutureWatcher<OperationResult>::finished, this, [this] {
        const auto result = watcher_->result();
        if (const auto *report = std::get_if<OperationReport>(&result))
            for (const auto &item : report->items) {
                if (item.succeeded)
                    retainedCleanup_.removeAll(item.source);
                for (const auto &path : item.cleanupPaths)
                    if (!retainedCleanup_.contains(path))
                        retainedCleanup_.push_back(path);
            }
        watcher_->deleteLater();
        watcher_ = nullptr;
        busy_ = false;
        emit busyChanged(false);
        emit finished(result);
    });
    emit busyChanged(true);
    const auto platform = platform_;
    const auto token = cancel_;
    watcher_->setFuture(QtConcurrent::run(&pool_, [this, platform, request, token]() -> OperationResult {
        QElapsedTimer throttle;
        throttle.start();
        QString lastPhase;
        const auto progress = [this, &throttle, &lastPhase](const OperationProgress &state) {
            if (state.phase != lastPhase || state.items == state.totalItems || throttle.elapsed() >= 50) {
                lastPhase = state.phase;
                throttle.restart();
                emit progressProduced(state);
            }
        };
        try {
            return platform->mutate(request, progress, token);
        } catch (const std::exception &e) {
            return Error{ErrorCode::Io, "File operation", {}, QString::fromLocal8Bit(e.what()), 0};
        }
    }));
    return true;
}
} // namespace explorer
