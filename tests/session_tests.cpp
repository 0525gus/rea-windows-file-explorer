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
    }
    std::shared_ptr<OpenReply> prepareOpen(const QString &key, Result<Unit> result, bool blocked = false) {
        auto reply = std::make_shared<OpenReply>(std::move(result), blocked);
        std::lock_guard guard(mutex_);
        openReplies_[key] = reply;
        allOpen_.push_back(reply);
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
};

QTEST_GUILESS_MAIN(SessionTests)
#include "session_tests.moc"
