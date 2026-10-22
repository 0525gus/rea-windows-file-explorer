#include "explorer/main_window.h"
#include "fake_platform.h"
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QTabBar>
#include <QTableView>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>

using namespace explorer;
class UiTest final : public QObject {
    Q_OBJECT
    QString fixture(const QString &suffix) const {
        return QDir::cleanPath(QString::fromUtf8(EXPLORER_FIXTURE_ROOT) + "/" + suffix);
    }
    void ready(MainWindow &window) {
        QTRY_VERIFY(window.controller()->current() && !window.controller()->current()->loading);
    }
  private slots:
    void addressAndFolderActivation() {
        MainWindow window;
        window.show();
        window.openInitial(Location::folder(fixture("read-only")));
        ready(window);
        QCOMPARE(window.fileModel()->rowCount(), 5);
        auto *address = window.findChild<QLineEdit *>("address");
        QVERIFY(address);
        address->setText(fixture("read-only/empty"));
        QTest::keyClick(address, Qt::Key_Return);
        ready(window);
        QCOMPARE(window.fileModel()->rowCount(), 0);
        QCOMPARE(window.controller()->current()->location.path, fixture("read-only/empty"));
        QTest::mouseClick(window.findChild<QToolButton *>("back"), Qt::LeftButton);
        ready(window);
        QCOMPARE(window.fileModel()->rowCount(), 5);
        auto *table = window.findChild<QTableView *>("fileList");
        QVERIFY(table);
        for (int row = 0; row < window.proxy()->rowCount(); ++row)
            if (window.proxy()->index(row, 0).data().toString() == "empty") {
                const auto index = window.proxy()->index(row, 0);
                table->scrollTo(index);
                QCoreApplication::processEvents();
                QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier,
                                  table->visualRect(index).center());
                QTest::mouseDClick(table->viewport(), Qt::LeftButton, Qt::NoModifier,
                                   table->visualRect(index).center());
                break;
            }
        QTRY_COMPARE(window.controller()->current()->location.path, fixture("read-only/empty"));
        ready(window);
    }
    void unicodeSearchAndColumns() {
        MainWindow window;
        window.show();
        window.openInitial(Location::folder(fixture("read-only/unicode")));
        ready(window);
        QCOMPARE(window.fileModel()->rowCount(), 6);
        QCOMPARE(window.fileModel()->columnCount(), 4);
        auto *search = window.findChild<QLineEdit *>("search");
        search->setText(QString::fromUtf8("한글"));
        QCOMPARE(window.proxy()->rowCount(), 2);
        QSet<QString> names;
        for (int row = 0; row < 2; ++row)
            names.insert(window.proxy()->index(row, 0).data().toString());
        QVERIFY(names.contains(QString::fromUtf8("한글_파일.txt")));
        QVERIFY(names.contains(QString::fromUtf8("한글_특수문자_é_😀.txt")));
        QVERIFY(window.findChild<QLabel *>("statusText")->text().contains("2 of 6"));
        search->clear();
        QCOMPARE(window.proxy()->rowCount(), 6);
    }
    void typedSorting() {
        MainWindow window;
        window.openInitial(Location::folder(fixture("read-only/unicode")));
        ready(window);
        auto *table = window.findChild<QTableView *>("fileList");
        for (const int column : {int(FileModel::Name), int(FileModel::Size), int(FileModel::Modified)}) {
            table->sortByColumn(column, Qt::AscendingOrder);
            const auto first = window.proxy()->mapToSource(window.proxy()->index(0, 0));
            const auto last = window.proxy()->mapToSource(window.proxy()->index(5, 0));
            if (column == FileModel::Size) {
                QCOMPARE(window.fileModel()->entry(first.row())->size, 25);
                QCOMPARE(window.fileModel()->entry(last.row())->size, 185);
            }
            if (column == FileModel::Modified)
                QVERIFY(window.fileModel()->entry(first.row())->modified <
                        window.fileModel()->entry(last.row())->modified);
            const auto firstName = window.proxy()->index(0, 0).data().toString();
            table->sortByColumn(column, Qt::DescendingOrder);
            QCOMPARE(window.proxy()->index(5, 0).data().toString(), firstName);
        }
    }
    void folderGroupingAndRefreshState() {
        auto fake = std::make_shared<FakePlatform>();
        const auto location = Location::folder("grouping");
        fake->set(location, Listing{location,
                                    {{location.path + "/folder", "folder", true, false, -1, {}, "Folder"},
                                     {location.path + "/file", "file", false, false, 10, {}, "File"}},
                                    {}});
        MainWindow window(fake);
        window.openInitial(location);
        ready(window);
        auto *table = window.findChild<QTableView *>("fileList");
        for (const auto direction : {Qt::AscendingOrder, Qt::DescendingOrder}) {
            table->sortByColumn(FileModel::Size, direction);
            QCOMPARE(window.proxy()->index(0, 0).data().toString(), QString("folder"));
        }
        window.findChild<QLineEdit *>("search")->setText("file");
        table->selectRow(0);
        window.controller()->refresh();
        ready(window);
        QCOMPARE(window.proxy()->rowCount(), 1);
        QCOMPARE(window.controller()->current()->query, QString("file"));
        QCOMPARE(window.controller()->current()->selection, QStringList{location.path + "/file"});
    }
    void invalidPathRetainsListAndInput() {
        MainWindow window;
        window.show();
        window.openInitial(Location::folder(fixture("read-only/unicode")));
        ready(window);
        const auto missing = fixture("does-not-exist");
        auto *address = window.findChild<QLineEdit *>("address");
        address->setText(missing);
        QTest::keyClick(address, Qt::Key_Return);
        ready(window);
        QCOMPARE(window.fileModel()->rowCount(), 6);
        QCOMPARE(address->text(), missing);
        QVERIFY(!window.findChild<QLabel *>("errorBanner")->isHidden());
        QTest::mouseClick(window.findChild<QToolButton *>("refresh"), Qt::LeftButton);
        ready(window);
        QVERIFY(window.findChild<QLabel *>("errorBanner")->isHidden());
    }
    void tabSwitchWhilePendingCommitsNewListing() {
        auto fake = std::make_shared<FakePlatform>();
        MainWindow window(fake);
        window.show();
        const auto a = Location::folder("a"), b = Location::folder("b");
        fake->set(a, Listing{a, {{a.path + "/one", "one", false, false, 1, {}, "File"}}, {}}, 120, true);
        fake->set(b, Listing{b, {{b.path + "/two", "two", false, false, 2, {}, "File"}}, {}});
        window.openInitial(a);
        const auto first = window.controller()->currentId();
        window.controller()->addTab(b);
        ready(window);
        window.controller()->setCurrent(first);
        ready(window);
        QCOMPARE(window.fileModel()->rowCount(), 1);
        QCOMPARE(window.fileModel()->entry(0)->name, QString("one"));
    }
    void tabsAndShortcuts() {
        MainWindow window;
        window.show();
        window.activateWindow();
        window.openInitial(Location::folder(fixture("read-only/empty")));
        ready(window);
        const auto first = window.controller()->currentId();
        QTest::keyClick(&window, Qt::Key_T, Qt::ControlModifier);
        QTRY_COMPARE(window.controller()->order().size(), 2);
        ready(window);
        QVERIFY(window.controller()->current()->location.computer);
        window.controller()->navigate(Location::folder(fixture("read-only/unicode")));
        ready(window);
        auto *tabs = window.findChild<QTabBar *>("tabs");
        tabs->setCurrentIndex(0);
        QCOMPARE(window.controller()->currentId(), first);
        QCOMPARE(window.fileModel()->rowCount(), 0);
        QTest::keyClick(&window, Qt::Key_W, Qt::ControlModifier);
        QTRY_COMPARE(window.controller()->order().size(), 1);
        QCOMPARE(window.fileModel()->rowCount(), 6);
    }
    void selectionStatusAndDefaultOpenDispatch() {
        auto fake = std::make_shared<FakePlatform>();
        MainWindow window(fake);
        window.show();
        const auto location = Location::folder("unicode");
        const QString name = QString::fromUtf8("한글_é_😀.txt");
        fake->set(
            location,
            Listing{location, {{location.path + "/" + name, name, false, false, 25, {}, "TXT file"}}, {}});
        window.openInitial(location);
        ready(window);
        auto *table = window.findChild<QTableView *>("fileList");
        table->selectRow(0);
        QVERIFY(window.findChild<QLabel *>("statusText")->text().contains("1 selected"));
        table->setCurrentIndex(window.proxy()->index(0, 0));
        QTest::mouseClick(window.findChild<QToolButton *>("open"), Qt::LeftButton);
        QCOMPARE(fake->opened, QStringList{location.path + "/" + name});
    }
    void bulkRemainsResponsive() {
        auto fake = std::make_shared<FakePlatform>();
        const auto bulk = Location::folder("bulk");
        Listing value{bulk, {}, {}};
        for (int i = 0; i < 1200; ++i)
            value.entries.push_back(
                {bulk.path + "/" + QString::number(i), QString::number(i), false, false, i, {}, "File"});
        fake->set(bulk, value, 150);
        MainWindow window(fake);
        window.show();
        int heartbeats = 0;
        QTimer timer;
        connect(&timer, &QTimer::timeout, &window, [&] { ++heartbeats; });
        timer.start(10);
        window.openInitial(bulk);
        ready(window);
        QVERIFY(heartbeats >= 3);
        QCOMPARE(window.fileModel()->rowCount(), 1200);
    }
    void treeNavigatesUsingAdapter() {
        auto fake = std::make_shared<FakePlatform>();
        MainWindow window(fake);
        window.show();
        const auto folder = Location::folder("tree-child");
        fake->set({}, Listing{{}, {{folder.path, "tree-child", true, false, -1, {}, "Folder"}}, {}});
        window.openInitial({});
        ready(window);
        auto *tree = window.findChild<QTreeWidget *>("navigationTree");
        auto *computer = tree->topLevelItem(1);
        computer->setExpanded(true);
        QTRY_COMPARE(computer->child(0)->text(0), QString("tree-child"));
        const auto rect = tree->visualItemRect(computer->child(0));
        QTest::mouseClick(tree->viewport(), Qt::LeftButton, Qt::NoModifier, rect.center());
        ready(window);
        QCOMPARE(window.controller()->current()->location, folder);
    }
    void restartAndResize() {
        for (int iteration = 0; iteration < 2; ++iteration) {
            MainWindow window;
            window.show();
            window.openInitial(Location::folder(fixture("read-only/empty")));
            ready(window);
            window.resize(800, 600);
            QTest::qWait(20);
            QVERIFY(window.findChild<QTableView *>("fileList")->width() > 200);
            QVERIFY(window.close());
        }
    }
};
QTEST_MAIN(UiTest)
#include "test_ui.moc"
