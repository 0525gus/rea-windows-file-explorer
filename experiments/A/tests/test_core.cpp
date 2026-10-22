#include "explorer/browser_controller.h"
#include "fake_platform.h"
#include <QSignalSpy>
#include <QTest>

using namespace explorer;
class CoreTest final : public QObject {
    Q_OBJECT
  private slots:
    void historyTransaction() {
        History history;
        const auto a = Location::folder("a"), b = Location::folder("b"), c = Location::folder("c");
        history.visit(a);
        history.visit(b);
        history.visit(b);
        QCOMPARE(history.size(), 2);
        QVERIFY(history.canBack());
        QVERIFY(!history.canForward());
        history.commitCursor(0);
        QCOMPARE(*history.at(history.cursor()), a);
        QVERIFY(history.canForward());
        history.visit(c);
        QCOMPARE(history.size(), 2);
        QVERIFY(!history.canForward());
        history.commitCursor(-1);
        QCOMPARE(history.cursor(), 1);
        QVERIFY(!history.at(2));
        QVERIFY(parentOf({}).computer);
    }
    void failurePreservesCommittedState() {
        auto fake = std::make_shared<FakePlatform>();
        BrowserController controller(fake);
        const auto good = Location::folder("good"), bad = Location::folder("bad");
        fake->set(good, Listing{good, {{good.path + "/x", "x", false, false, 12, {}, "File"}}, {}});
        fake->set(bad, Error{ErrorCode::AccessDenied, "Open folder", bad.path, "Denied", 5});
        controller.addTab(good);
        QTRY_VERIFY(!controller.current()->loading);
        controller.navigate(bad);
        QTRY_VERIFY(!controller.current()->loading);
        QCOMPARE(controller.current()->location, good);
        QCOMPARE(controller.current()->listing.entries.size(), 1);
        QCOMPARE(controller.current()->history.size(), 1);
        QVERIFY(controller.current()->error);
        QCOMPARE(controller.current()->error->code, ErrorCode::AccessDenied);
    }
    void backForwardUpRefresh() {
        auto fake = std::make_shared<FakePlatform>();
        BrowserController controller(fake);
        const auto a = Location::folder("navigation"), b = Location::folder("navigation/child");
        controller.addTab(a);
        QTRY_VERIFY(!controller.current()->loading);
        controller.navigate(b);
        QTRY_VERIFY(!controller.current()->loading);
        controller.back();
        QTRY_VERIFY(!controller.current()->loading);
        QCOMPARE(controller.current()->location, a);
        controller.forward();
        QTRY_VERIFY(!controller.current()->loading);
        QCOMPARE(controller.current()->location, b);
        controller.refresh();
        QTRY_VERIFY(!controller.current()->loading);
        QCOMPARE(controller.current()->history.size(), 2);
        controller.up();
        QTRY_VERIFY(!controller.current()->loading);
        QCOMPARE(controller.current()->location, a);
        controller.back();
        QTRY_VERIFY(!controller.current()->loading);
        QCOMPARE(controller.current()->location, b);
    }
    void failedHistoryDoesNotMoveCursor() {
        auto fake = std::make_shared<FakePlatform>();
        BrowserController controller(fake);
        const auto a = Location::folder("a"), b = Location::folder("b");
        controller.addTab(a);
        QTRY_VERIFY(!controller.current()->loading);
        controller.navigate(b);
        QTRY_VERIFY(!controller.current()->loading);
        fake->set(a, Error{ErrorCode::NotFound, "Open", a.path, "Gone", 2});
        controller.back();
        QTRY_VERIFY(!controller.current()->loading);
        QCOMPARE(controller.current()->history.cursor(), 1);
        QCOMPARE(controller.current()->location, b);
    }
    void independentTabsAndDefault() {
        auto fake = std::make_shared<FakePlatform>();
        BrowserController controller(fake);
        const auto a = Location::folder("a"), b = Location::folder("b");
        const auto first = controller.addTab(a);
        QTRY_VERIFY(!controller.current()->loading);
        controller.setQuery("needle");
        controller.setSort(3, Qt::DescendingOrder);
        const auto second = controller.addTab();
        QTRY_VERIFY(!controller.current()->loading);
        QVERIFY(controller.current()->location.computer);
        controller.navigate(b);
        QTRY_VERIFY(!controller.current()->loading);
        controller.setCurrent(first);
        QCOMPARE(controller.current()->location, a);
        QCOMPARE(controller.current()->query, QString("needle"));
        QCOMPARE(controller.current()->sortColumn, 3);
        QCOMPARE(controller.tab(second)->location, b);
        controller.closeTab(second);
        QCOMPARE(controller.order().size(), 1);
        controller.closeTab(first);
        QTRY_VERIFY(!controller.current()->loading);
        QCOMPARE(controller.order().size(), 1);
        QVERIFY(controller.current()->location.computer);
    }
    void lateCompletionIsDiscarded() {
        auto fake = std::make_shared<FakePlatform>();
        BrowserController controller(fake);
        const auto slow = Location::folder("slow"), fast = Location::folder("fast");
        fake->set(slow, Listing{slow, {}, {}}, 150, true);
        controller.addTab(slow);
        controller.navigate(fast);
        QTRY_VERIFY(!controller.current()->loading);
        QCOMPARE(controller.current()->location, fast);
        QTest::qWait(200);
        QCOMPARE(controller.current()->location, fast);
        QCOMPARE(controller.current()->history.size(), 1);
    }
    void closeDuringListing() {
        auto fake = std::make_shared<FakePlatform>();
        BrowserController controller(fake);
        const auto slow = Location::folder("slow");
        fake->set(slow, Listing{slow, {}, {}}, 100, true);
        const auto first = controller.addTab(slow);
        controller.addTab();
        controller.closeTab(first);
        QTest::qWait(160);
        QCOMPARE(controller.order().size(), 1);
        QVERIFY(controller.current()->location.computer);
    }
    void shutdownOwnsWorker() {
        auto fake = std::make_shared<FakePlatform>();
        const auto slow = Location::folder("slow");
        fake->set(slow, Listing{slow, {}, {}}, 80, true);
        {
            BrowserController controller(fake);
            controller.addTab(slow);
        }
        QVERIFY(true); // Destructor waits for the real worker; ASan-free lifecycle smoke, not a timing claim.
    }
    void fileActivationUsesPlatform() {
        auto fake = std::make_shared<FakePlatform>();
        BrowserController controller(fake);
        controller.addTab();
        QTRY_VERIFY(!controller.current()->loading);
        const QString path = QString::fromUtf8("/fixture/한글_é_😀.txt");
        controller.activate({path, "name", false, false, 1, {}, "File"});
        QCOMPARE(fake->opened, QStringList{path});
        QVERIFY(!controller.current()->error);
        QVERIFY(controller.capabilities().open);
        QVERIFY(!controller.capabilities().nativeProperties);
    }
};
QTEST_GUILESS_MAIN(CoreTest)
#include "test_core.moc"
