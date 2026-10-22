#pragma once
#include "platform.h"
#include <QFutureWatcher>
#include <QObject>
#include <QThreadPool>
namespace explorer {
class OperationManager final : public QObject {
    Q_OBJECT
  public:
    explicit OperationManager(std::shared_ptr<Platform>, QObject *parent = nullptr);
    ~OperationManager() override;
    bool start(const OperationRequest &);
    bool busy() const {
        return busy_;
    }
    void cancel();
    const QStringList &retainedCleanup() const {
        return retainedCleanup_;
    }
  signals:
    void busyChanged(bool);
    void progressChanged(const explorer::OperationProgress &);
    void finished(const explorer::OperationResult &);
    void progressProduced(const explorer::OperationProgress &);

  private:
    std::shared_ptr<Platform> platform_;
    QThreadPool pool_;
    QFutureWatcher<OperationResult> *watcher_ = nullptr;
    CancelToken cancel_ = cancellation();
    bool busy_ = false;
    QStringList retainedCleanup_;
};
} // namespace explorer
Q_DECLARE_METATYPE(explorer::OperationProgress)
Q_DECLARE_METATYPE(explorer::OperationResult)
