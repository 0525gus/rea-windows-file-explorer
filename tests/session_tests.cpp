#include "explorer/session.hpp"
#include <QSignalSpy>
#include <QTest>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>

using namespace explorer;

namespace {
Location loc(const QString &key) {
    Location value;
    value.kind = Location::Kind::Directory;
    value.callerInput = key;
    value.path = key;
    value.key = key;
    value.label = key;
    return value;
}
Entry item(const QString &key, bool directory = false, QString identity = QStringLiteral("identity")) {
    Entry value;
    value.key = key;
    value.path = key;
    value.name = key;
    value.identity = std::move(identity);
    value.directory = directory;
    return value;
}
Listing listing(const QString &key, QVector<Entry> entries = {}) {
    return {loc(key), std::move(entries), true};
}
Error problem(ErrorCode code, const QString &input) {
    return {code,  QStringLiteral("list"),
            input, QStringLiteral("Controlled adapter failure"),
            37,    {QStringLiteral("partial-evidence")}};
}

template <class T> struct ControlledReply {
    explicit ControlledReply(Result<T> answer, bool blocked = false)
        : result(std::move(answer)), released(!blocked) {}
    Result<T> result;
    std::atomic<bool> started{false}, returned{false}, stopped{false};
    std::mutex mutex;
    std::condition_variable ready;
    bool released;
    void release() {
        {
            std::lock_guard guard(mutex);
            released = true;
        }
        ready.notify_all();
    }
};
using Reply = ControlledReply<Listing>;
using OpenReply = ControlledReply<Unit>;
using JobReply = ControlledReply<OperationReport>;

class ControlledPlatform final : public IPlatform {
  public:
    std::shared_ptr<Reply> prepare(const QString &path, Result<Listing> value, bool blocked = false) {
        auto reply = std::make_shared<Reply>(std::move(value), blocked);
        std::lock_guard guard(mutex_);
        replies_[path] = reply;
        all_.push_back(reply);
        return reply;
    }
    void releaseAll() {
        std::lock_guard guard(mutex_);
        for (const auto &reply : all_)
            reply->release();
        for (const auto &reply : allOpen_)
            reply->release();
        for (const auto &reply : allJobs_)
            reply->release();
        for (const auto &reply : allCleanup_)
            reply->release();
    }
    std::shared_ptr<OpenReply> prepareOpen(const QString &key, Result<Unit> result, bool blocked = false) {
        auto reply = std::make_shared<OpenReply>(std::move(result), blocked);
        std::lock_guard guard(mutex_);
        openReplies_[key] = reply;
        allOpen_.push_back(reply);
        return reply;
    }
    std::shared_ptr<JobReply> prepareOperation(OperationReport report, bool blocked = false) {
        auto reply = std::make_shared<JobReply>(std::move(report), blocked);
        std::lock_guard guard(mutex_);
        nextJob_ = reply;
        allJobs_.push_back(reply);
        return reply;
    }
    std::shared_ptr<OpenReply> prepareCleanup(const QString &path, Result<Unit> result,
                                              bool blocked = false) {
        auto reply = std::make_shared<OpenReply>(std::move(result), blocked);
        std::lock_guard guard(mutex_);
        cleanupReplies_[path] = reply;
        allCleanup_.push_back(reply);
        return reply;
    }
    Result<Location> parseLocation(const QString &input) const override {
        if (input == QStringLiteral("bad-input"))
            return problem(ErrorCode::InvalidPath, input);
        return loc(input);
    }
    Result<std::optional<Location>> parent(const Location &location) const override {
        if (location.key == QStringLiteral("parent-fails"))
            return problem(ErrorCode::PermissionDenied, location.callerInput);
        if (location.key == QStringLiteral("root"))
            return std::optional<Location>{};
        return std::optional<Location>{loc(QStringLiteral("root"))};
    }
    Result<Listing> list(const Location &location, std::stop_token stop,
                         ProgressSink progress) const override {
        std::shared_ptr<Reply> reply;
        {
            std::lock_guard guard(mutex_);
            const auto found = replies_.find(location.key);
            if (found == replies_.end())
                return problem(ErrorCode::NotFound, location.callerInput);
            reply = found->second;
        }
        reply->started = true;
        if (progress)
            progress({17, QStringLiteral("controlled-list")});
        {
            std::unique_lock guard(reply->mutex);
            reply->ready.wait(guard, [&] { return reply->released; });
        }
        // Intentionally allow a successful response after cancellation. A queued
        // OS call may complete late, so the session must reject it by generation.
        reply->stopped = stop.stop_requested();
        reply->returned = true;
        return reply->result;
    }
    int compareNames(const QString &a, const QString &b) const override {
        return QString::compare(a, b, Qt::CaseInsensitive);
    }
    Result<Unit> open(const Entry &entry) const override {
        std::shared_ptr<OpenReply> reply;
        {
            std::lock_guard guard(mutex_);
            opened_.push_back(entry);
            const auto found = openReplies_.find(entry.key);
            if (found != openReplies_.end())
                reply = found->second;
        }
        if (reply) {
            reply->started = true;
            {
                std::unique_lock guard(reply->mutex);
                reply->ready.wait(guard, [&] { return reply->released; });
            }
            reply->returned = true;
            return reply->result;
        }
        if (entry.key == QStringLiteral("removed"))
            return problem(ErrorCode::NotFound, entry.path);
        return Unit{};
    }
    QVector<Entry> opened() const {
        std::lock_guard guard(mutex_);
        return opened_;
    }
    OperationReport execute(const OperationRequest &request, std::stop_token stop,
                            OperationProgressSink progress) const override {
        std::shared_ptr<JobReply> reply;
        {
            std::lock_guard guard(mutex_);
            requests_.push_back(request);
            reply = nextJob_;
        }
        if (!reply)
            return {request.kind,
                    {{QStringLiteral("unexpected"),
                      {},
                      ItemStatus::Failed,
                      problem(ErrorCode::Unsupported, {})}}};
        reply->started = true;
        if (progress)
            progress({0, quint64(request.sources.size()), 7, 21, QStringLiteral("controlled-source"),
                      QStringLiteral("copy")});
        {
            std::unique_lock guard(reply->mutex);
            reply->ready.wait(guard, [&] { return reply->released; });
        }
        reply->stopped = stop.stop_requested();
        reply->returned = true;
        return std::get<OperationReport>(reply->result);
    }
    Result<Unit> retryCleanup(const QString &path) const override {
        std::shared_ptr<OpenReply> reply;
        {
            std::lock_guard guard(mutex_);
            cleanupCalls_.push_back(path);
            const auto found = cleanupReplies_.find(path);
            if (found != cleanupReplies_.end())
                reply = found->second;
        }
        if (!reply)
            return problem(ErrorCode::Unsupported, path);
        reply->started = true;
        {
            std::unique_lock guard(reply->mutex);
            reply->ready.wait(guard, [&] { return reply->released; });
        }
        reply->returned = true;
        return reply->result;
    }
    QVector<OperationRequest> requests() const {
        std::lock_guard guard(mutex_);
        return requests_;
    }
    QStringList cleanupCalls() const {
        std::lock_guard guard(mutex_);
        return cleanupCalls_;
    }
    Capabilities capabilities() const override {
        return {true, false, false, false, false, false};
    }

  private:
    mutable std::mutex mutex_;
    std::map<QString, std::shared_ptr<Reply>> replies_;
    QVector<std::shared_ptr<Reply>> all_;
    std::map<QString, std::shared_ptr<OpenReply>> openReplies_;
    QVector<std::shared_ptr<OpenReply>> allOpen_;
    mutable QVector<Entry> opened_;
    std::shared_ptr<JobReply> nextJob_;
    QVector<std::shared_ptr<JobReply>> allJobs_;
    std::map<QString, std::shared_ptr<OpenReply>> cleanupReplies_;
    QVector<std::shared_ptr<OpenReply>> allCleanup_;
    mutable QVector<OperationRequest> requests_;
    mutable QStringList cleanupCalls_;
};

struct Harness {
    std::shared_ptr<ControlledPlatform> platform = std::make_shared<ControlledPlatform>();
    std::unique_ptr<ExplorerSession> session = std::make_unique<ExplorerSession>(platform);
    ~Harness() {
        // Always release controlled work before the session's pool is destroyed,
        // including early QVERIFY returns. A failing test cannot deadlock cleanup.
        platform->releaseAll();
        session->shutdown();
        const bool drained = QTest::qWaitFor([&] { return session->idle(); }, 5000);
        if (!drained)
            QTest::qFail("Controlled session work did not drain during test cleanup.", __FILE__, __LINE__);
    }
};
QStringList names(const QVector<Entry> &entries) {
    QStringList result;
    for (const auto &e : entries)
        result.push_back(e.name);
    return result;
}
} // namespace

class SessionTests final : public QObject {
    Q_OBJECT
  private slots:
    void successfulHistoryFailurePreservationAndBranch() {
        Harness h;
        h.platform->prepare(QStringLiteral("A"),
                            listing(QStringLiteral("A"), {item(QStringLiteral("original"))}));
        h.platform->prepare(QStringLiteral("B"), listing(QStringLiteral("B")));
        h.platform->prepare(QStringLiteral("C"), listing(QStringLiteral("C")));
        const auto id = h.session->createTab(loc(QStringLiteral("A")));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        h.session->navigate(id, QStringLiteral("B"));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        h.session->navigate(id, QStringLiteral("C"));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QCOMPARE(h.session->tab(id)->history.cursor(), 2);

        h.platform->prepare(QStringLiteral("B"), problem(ErrorCode::PermissionDenied, QStringLiteral("B")));
        h.session->back(id);
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QCOMPARE(h.session->tab(id)->location.key, QStringLiteral("C"));
        QCOMPARE(h.session->tab(id)->history.cursor(), 2);
        QVERIFY(h.session->tab(id)->error);
        QCOMPARE(h.session->tab(id)->error->code, ErrorCode::PermissionDenied);
        QCOMPARE(h.session->tab(id)->error->nativeCode, 37);
        QCOMPARE(h.session->tab(id)->error->partialPaths, QStringList{QStringLiteral("partial-evidence")});

        h.platform->prepare(QStringLiteral("B"), listing(QStringLiteral("B")));
        h.session->back(id);
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QCOMPARE(h.session->tab(id)->history.cursor(), 1);
        QCOMPARE(h.session->tab(id)->history.locations().size(), 3);
        h.session->refresh(id);
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QCOMPARE(h.session->tab(id)->history.locations().size(), 3);
        h.platform->prepare(QStringLiteral("D"), listing(QStringLiteral("D")));
        h.session->navigate(id, QStringLiteral("D"));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QCOMPARE(h.session->tab(id)->history.locations().size(), 3);
        QCOMPARE(h.session->tab(id)->history.at(2)->key, QStringLiteral("D"));
        QVERIFY(!h.session->tab(id)->history.canForward());
    }

    void pendingParseFailureAndIncompleteListingKeepCommittedState() {
        Harness h;
        h.platform->prepare(QStringLiteral("A"),
                            listing(QStringLiteral("A"), {item(QStringLiteral("kept"))}));
        const auto id = h.session->createTab(loc(QStringLiteral("A")));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        const auto slow =
            h.platform->prepare(QStringLiteral("pending"), listing(QStringLiteral("pending")), true);
        h.session->navigate(id, QStringLiteral("pending"));
        QTRY_VERIFY_WITH_TIMEOUT(slow->started.load(), 5000);
        QCOMPARE(h.session->tab(id)->location.key, QStringLiteral("A"));
        QCOMPARE(names(h.session->tab(id)->entries), QStringList{QStringLiteral("kept")});
        QCOMPARE(h.session->tab(id)->history.cursor(), 0);
        QCOMPARE(h.session->tab(id)->examined->load(), quint64(17));
        h.session->navigate(id, QStringLiteral("bad-input"));
        slow->release();
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QVERIFY(h.session->tab(id)->error);
        QCOMPARE(h.session->tab(id)->error->code, ErrorCode::InvalidPath);
        QCOMPARE(h.session->tab(id)->location.key, QStringLiteral("A"));
        auto partial = listing(QStringLiteral("partial"), {item(QStringLiteral("must-not-commit"))});
        partial.complete = false;
        h.platform->prepare(QStringLiteral("partial"), partial);
        h.session->navigate(id, QStringLiteral("partial"));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QVERIFY(h.session->tab(id)->error.has_value());
        QCOMPARE(h.session->tab(id)->location.key, QStringLiteral("A"));
        QCOMPARE(names(h.session->tab(id)->entries), QStringList{QStringLiteral("kept")});
        QCOMPARE(h.session->tab(id)->history.locations().size(), 1);
    }

    void supersededLateSuccessAndLateFailureCannotOverwriteCurrent() {
        for (const bool lateFailure : {false, true}) {
            Harness h;
            h.platform->prepare(QStringLiteral("A"), listing(QStringLiteral("A")));
            const auto id = h.session->createTab(loc(QStringLiteral("A")));
            QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
            const auto late = h.platform->prepare(
                QStringLiteral("late"),
                lateFailure ? Result<Listing>{problem(ErrorCode::Io, QStringLiteral("late"))}
                            : Result<Listing>{listing(QStringLiteral("late"))},
                true);
            h.platform->prepare(QStringLiteral("new"),
                                listing(QStringLiteral("new"), {item(QStringLiteral("current"))}));
            h.session->navigate(id, QStringLiteral("late"));
            QTRY_VERIFY_WITH_TIMEOUT(late->started.load(), 5000);
            h.session->navigate(id, QStringLiteral("new"));
            late->release();
            QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
            QVERIFY(late->stopped.load());
            QCOMPARE(h.session->tab(id)->location.key, QStringLiteral("new"));
            QCOMPARE(names(h.session->tab(id)->entries), QStringList{QStringLiteral("current")});
            QVERIFY(!h.session->tab(id)->error);
            QCOMPARE(h.session->tab(id)->history.locations().size(), 2);
        }
    }

    void tabsOwnIndependentHistorySearchSortAndSelection() {
        Harness h;
        h.platform->prepare(
            QStringLiteral("A"),
            listing(QStringLiteral("A"), {item(QStringLiteral("alpha")), item(QStringLiteral("zeta"))}));
        h.platform->prepare(QStringLiteral("B"),
                            listing(QStringLiteral("B"), {item(QStringLiteral("beta"))}));
        h.platform->prepare(QStringLiteral("C"),
                            listing(QStringLiteral("C"), {item(QStringLiteral("gamma"))}));
        const auto a = h.session->createTab(loc(QStringLiteral("A")));
        const auto b = h.session->createTab(loc(QStringLiteral("B")));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        h.session->setSelection(a, {QStringLiteral("alpha")});
        h.session->setSearch(a, QStringLiteral("ALP"));
        h.session->setSort(a, {SortColumn::Name, false});
        h.session->navigate(b, QStringLiteral("C"));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QCOMPARE(h.session->tab(a)->location.key, QStringLiteral("A"));
        QCOMPARE(h.session->tab(a)->query, QStringLiteral("ALP"));
        QVERIFY(!h.session->tab(a)->sort.ascending);
        QCOMPARE(h.session->tab(a)->selection, QSet<QString>{QStringLiteral("alpha")});
        QCOMPARE(h.session->tab(a)->history.locations().size(), 1);
        QCOMPARE(h.session->tab(b)->history.locations().size(), 2);
        QCOMPARE(h.session->tab(b)->query, QString{});
        QVERIFY(h.session->tab(b)->selection.isEmpty());
        h.session->refresh(a);
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QCOMPARE(h.session->tab(a)->query, QStringLiteral("ALP"));
        QCOMPARE(names(h.session->tab(a)->visible), QStringList{QStringLiteral("alpha")});
        h.session->navigate(a, QStringLiteral("C"));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QCOMPARE(h.session->tab(a)->query, QString{});
        QCOMPARE(h.session->tab(a)->location.key, QStringLiteral("C"));
        QCOMPARE(names(h.session->tab(a)->visible), QStringList{QStringLiteral("gamma")});
        QCOMPARE(h.session->tab(b)->location.key, QStringLiteral("C"));
        QCOMPARE(h.session->tab(b)->history.locations().size(), 2);
    }

    void refreshSelectionRequiresMatchingKnownIdentity() {
        Harness h;
        h.platform->prepare(
            QStringLiteral("A"),
            listing(QStringLiteral("A"), {item(QStringLiteral("same"), false, QStringLiteral("1")),
                                          item(QStringLiteral("replaced"), false, QStringLiteral("old")),
                                          item(QStringLiteral("unknown"), false, {})}));
        const auto id = h.session->createTab(loc(QStringLiteral("A")));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        h.session->setSelection(
            id, {QStringLiteral("same"), QStringLiteral("replaced"), QStringLiteral("unknown")});
        h.platform->prepare(
            QStringLiteral("A"),
            listing(QStringLiteral("A"), {item(QStringLiteral("same"), false, QStringLiteral("1")),
                                          item(QStringLiteral("replaced"), false, QStringLiteral("new")),
                                          item(QStringLiteral("unknown"), false, {})}));
        h.session->refresh(id);
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QCOMPARE(h.session->tab(id)->selection, QSet<QString>{QStringLiteral("same")});
        h.session->setSearch(id, QStringLiteral("absent"));
        QVERIFY(h.session->tab(id)->selection.isEmpty());
    }

    void safeOpenDelegatesOnlyVisibleFileAndReportsRemovedItem() {
        Harness h;
        h.platform->prepare(
            QStringLiteral("A"),
            listing(QStringLiteral("A"), {item(QStringLiteral("file")), item(QStringLiteral("removed")),
                                          item(QStringLiteral("folder"), true)}));
        h.platform->prepare(QStringLiteral("folder"), listing(QStringLiteral("folder")));
        const auto id = h.session->createTab(loc(QStringLiteral("A")));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        h.session->openEntry(id, QStringLiteral("file"));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QCOMPARE(h.platform->opened().size(), 1);
        QCOMPARE(h.platform->opened().first().key, QStringLiteral("file"));
        h.session->setSearch(id, QStringLiteral("removed"));
        h.session->openEntry(id, QStringLiteral("file"));
        QCOMPARE(h.platform->opened().size(), 1);
        h.session->openEntry(id, QStringLiteral("removed"));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QVERIFY(h.session->tab(id)->error);
        QCOMPARE(h.session->tab(id)->error->code, ErrorCode::NotFound);
        QCOMPARE(h.session->tab(id)->location.key, QStringLiteral("A"));
        h.session->setSearch(id, {});
        h.session->openEntry(id, QStringLiteral("folder"));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QCOMPARE(h.session->tab(id)->location.key, QStringLiteral("folder"));
        QCOMPARE(h.platform->opened().size(), 2); // No OS application was launched.
    }

    void delayedOpenIsOwnedThroughNavigationCloseAndShutdown() {
        Harness h;
        h.platform->prepare(QStringLiteral("A"),
                            listing(QStringLiteral("A"), {item(QStringLiteral("slow-file"))}));
        h.platform->prepare(QStringLiteral("B"),
                            listing(QStringLiteral("B"), {item(QStringLiteral("shutdown-file"))}));
        const auto late = h.platform->prepareOpen(
            QStringLiteral("slow-file"), problem(ErrorCode::NotFound, QStringLiteral("slow-file")), true);
        const auto id = h.session->createTab(loc(QStringLiteral("A")));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        h.session->openEntry(id, QStringLiteral("slow-file"));
        QTRY_VERIFY_WITH_TIMEOUT(late->started.load(), 5000);
        QVERIFY(!h.session->idle());
        h.session->navigate(id, QStringLiteral("B"));
        late->release();
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QCOMPARE(h.session->tab(id)->location.key, QStringLiteral("B"));
        QVERIFY(!h.session->tab(id)->error); // Old open error cannot poison B.

        const auto pending = h.platform->prepareOpen(QStringLiteral("shutdown-file"), Unit{}, true);
        h.session->openEntry(id, QStringLiteral("shutdown-file"));
        QTRY_VERIFY_WITH_TIMEOUT(pending->started.load(), 5000);
        QSignalSpy shutdown(h.session.get(), &ExplorerSession::shutdownFinished);
        h.session->shutdown();
        h.session->closeTab(id);
        QVERIFY(!h.session->idle());
        QCOMPARE(shutdown.count(), 0);
        pending->release();
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(shutdown.count(), 1, 5000);
        QVERIFY(pending->returned.load());
        QVERIFY(!h.session->tab(id));
        QCOMPARE(h.platform->opened().size(), 2); // Both opens were controlled doubles.
    }

    void closedTabWorkerIsRetainedAndShutdownWaitsForActualReturn() {
        Harness h;
        const auto delayed =
            h.platform->prepare(QStringLiteral("late"), listing(QStringLiteral("late")), true);
        const auto id = h.session->createTab(loc(QStringLiteral("late")));
        QTRY_VERIFY_WITH_TIMEOUT(delayed->started.load(), 5000);
        QSignalSpy shutdown(h.session.get(), &ExplorerSession::shutdownFinished);
        h.session->closeTab(id);
        QVERIFY(!h.session->tab(id));
        QVERIFY(!h.session->idle());
        h.session->shutdown();
        QCOMPARE(h.session->createTab(), quint64(0));
        QCOMPARE(shutdown.count(), 0);
        delayed->release();
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(shutdown.count(), 1, 5000);
        QVERIFY(delayed->returned.load());
        QVERIFY(delayed->stopped.load());
        QVERIFY(h.session->tabs().isEmpty());
        h.session->shutdown();
        QTest::qWait(10);
        QCOMPARE(shutdown.count(), 1);
    }

    void globalOperationSurvivesTabCloseNavigationAndPreservesMixedReport() {
        Harness h;
        h.platform->prepare(QStringLiteral("A"), listing(QStringLiteral("A")));
        h.platform->prepare(QStringLiteral("B"), listing(QStringLiteral("B")));
        h.platform->prepare(QStringLiteral("C"),
                            listing(QStringLiteral("C"), {item(QStringLiteral("current"))}));
        const auto a = h.session->createTab(loc(QStringLiteral("A")));
        const auto b = h.session->createTab(loc(QStringLiteral("B")));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        OperationReport expected;
        expected.kind = OperationKind::Copy;
        expected.items = {{QStringLiteral("one"), QStringLiteral("target/one"), ItemStatus::Succeeded},
                          {QStringLiteral("two"), QStringLiteral("target/two"), ItemStatus::Failed,
                           problem(ErrorCode::Conflict, QStringLiteral("two"))}};
        const auto job = h.platform->prepareOperation(expected, true);
        const OperationRequest request{OperationKind::Copy,
                                       {item(QStringLiteral("one")), item(QStringLiteral("two"))},
                                       loc(QStringLiteral("target"))};
        QSignalSpy changed(h.session.get(), &ExplorerSession::operationChanged);
        QVERIFY(h.session->beginOperation(request));
        QTRY_VERIFY_WITH_TIMEOUT(job->started.load(), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(h.session->operationState().progress.completedBytes, quint64(7), 5000);
        QVERIFY(!h.session->beginOperation(request));
        h.session->closeTab(a);
        h.session->navigate(b, QStringLiteral("C"));
        QTRY_COMPARE_WITH_TIMEOUT(h.session->tab(b)->location.key, QStringLiteral("C"), 5000);
        QVERIFY(!h.session->tab(a));
        QVERIFY(h.session->operationState().running);
        QVERIFY(!h.session->idle());
        QCOMPARE(h.platform->requests().size(), 1);
        QCOMPARE(h.platform->requests().first().sources.size(), 2);
        QCOMPARE(h.platform->requests().first().sources.first().identity, QStringLiteral("identity"));
        job->release();
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QVERIFY(job->returned.load());
        QVERIFY(!job->stopped.load());
        const auto final = h.session->operationState();
        QVERIFY(!final.running && final.report);
        QCOMPARE(final.report->kind, OperationKind::Copy);
        QCOMPARE(final.report->items.size(), 2);
        QCOMPARE(final.report->items[0].status, ItemStatus::Succeeded);
        QCOMPARE(final.report->items[1].status, ItemStatus::Failed);
        QVERIFY(final.report->items[1].error);
        QCOMPARE(final.report->items[1].error->code, ErrorCode::Conflict);
        QCOMPARE(final.report->items[1].error->nativeCode, 37);
        QCOMPARE(final.report->items[1].error->partialPaths, QStringList{QStringLiteral("partial-evidence")});
        QCOMPARE(h.session->tab(b)->location.key, QStringLiteral("C"));
        QCOMPARE(names(h.session->tab(b)->entries), QStringList{QStringLiteral("current")});
        QVERIFY(changed.count() >= 2);
    }

    void cancelledOperationRemainsOwnedThroughShutdownAndLateCompletion() {
        Harness h;
        OperationReport expected;
        expected.kind = OperationKind::Move;
        expected.cancelled = true;
        expected.items = {{QStringLiteral("one"),
                           {},
                           ItemStatus::Cancelled,
                           problem(ErrorCode::Cancelled, QStringLiteral("one")),
                           {QStringLiteral("owned-stage")}}};
        expected.cleanupPending = {QStringLiteral("owned-stage")};
        const auto job = h.platform->prepareOperation(expected, true);
        const OperationRequest request{
            OperationKind::Move, {item(QStringLiteral("one"))}, loc(QStringLiteral("target"))};
        QVERIFY(h.session->beginOperation(request));
        QTRY_VERIFY_WITH_TIMEOUT(job->started.load(), 5000);
        h.session->cancelOperation();
        QSignalSpy shutdown(h.session.get(), &ExplorerSession::shutdownFinished);
        h.session->shutdown();
        h.session->shutdown();
        QVERIFY(!h.session->idle());
        QVERIFY(h.session->operationState().running);
        QVERIFY(!job->returned.load());
        QCOMPARE(shutdown.count(), 0);
        QVERIFY(!h.session->beginOperation(request));
        job->release();
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(shutdown.count(), 1, 5000);
        QVERIFY(job->stopped.load() && job->returned.load());
        const auto final = h.session->operationState();
        QVERIFY(!final.running && final.report);
        QVERIFY(final.report->cancelled);
        QCOMPARE(final.report->items.size(), 1);
        QCOMPARE(final.report->items.first().status, ItemStatus::Cancelled);
        QCOMPARE(final.report->cleanupPending, QStringList{QStringLiteral("owned-stage")});
        QCOMPARE(final.report->items.first().retainedPaths, QStringList{QStringLiteral("owned-stage")});
        h.session->shutdown();
        QTest::qWait(10);
        QCOMPARE(shutdown.count(), 1);
    }

    void cleanupRetryRetainsFailureRejectsForeignPathAndOwnsFinalization() {
        Harness h;
        const auto owned = QStringLiteral("exclusive-owned-stage");
        OperationReport expected;
        expected.items = {{QStringLiteral("source"),
                           {},
                           ItemStatus::Partial,
                           problem(ErrorCode::Io, QStringLiteral("source")),
                           {owned}}};
        expected.cleanupPending = {owned};
        h.platform->prepareOperation(expected);
        const OperationRequest request{
            OperationKind::Copy, {item(QStringLiteral("source"))}, loc(QStringLiteral("target"))};
        QVERIFY(h.session->beginOperation(request));
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QVERIFY(!h.session->beginOperation(request));
        h.session->retryOperationCleanup(QStringLiteral("foreign-path"));
        QCOMPARE(h.platform->cleanupCalls().size(), 0);
        const auto failed =
            h.platform->prepareCleanup(owned, problem(ErrorCode::PermissionDenied, owned), true);
        h.session->retryOperationCleanup(owned);
        QTRY_VERIFY_WITH_TIMEOUT(failed->started.load(), 5000);
        QVERIFY(!h.session->idle());
        QVERIFY(h.session->operationState().running);
        h.session->retryOperationCleanup(owned);
        QCOMPARE(h.platform->cleanupCalls(), QStringList{owned});
        failed->release();
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        auto state = h.session->operationState();
        QVERIFY(state.report);
        QCOMPARE(state.report->cleanupPending, QStringList{owned});
        QVERIFY(
            std::any_of(state.report->items.begin(), state.report->items.end(), [&](const ItemOutcome &item) {
                return item.error && item.error->code == ErrorCode::PermissionDenied &&
                       item.retainedPaths.contains(owned);
            }));
        QVERIFY(!h.session->beginOperation(request));
        const auto success = h.platform->prepareCleanup(owned, Unit{}, true);
        h.session->retryOperationCleanup(owned);
        QTRY_VERIFY_WITH_TIMEOUT(success->started.load(), 5000);
        QSignalSpy shutdown(h.session.get(), &ExplorerSession::shutdownFinished);
        h.session->shutdown();
        QVERIFY(!h.session->idle());
        QCOMPARE(shutdown.count(), 0);
        success->release();
        QTRY_VERIFY_WITH_TIMEOUT(h.session->idle(), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(shutdown.count(), 1, 5000);
        QVERIFY(success->returned.load());
        state = h.session->operationState();
        QVERIFY(state.report && state.report->cleanupPending.isEmpty());
        QCOMPARE(h.platform->cleanupCalls(), (QStringList{owned, owned}));
        QCOMPARE(state.report->items.first().retainedPaths,
                 QStringList{owned}); // Earlier evidence remains intact.
    }
};

QTEST_GUILESS_MAIN(SessionTests)
#include "session_tests.moc"
