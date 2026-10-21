#include "explorer/main_window.hpp"
#include <QAbstractItemModel>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QTabBar>
#include <QTableView>
#include <QTest>
#include <QToolButton>
#include <QTreeWidget>
#include <algorithm>

using namespace explorer;

class UiTests final : public QObject {
    Q_OBJECT
    QString repository_, fixture_;
    QJsonObject manifest_;
    std::unique_ptr<MainWindow> window_;

    QString path(const QString &relative) const {
        return QDir(fixture_).filePath(relative);
    }
    QTableView *table() const {
        return window_->findChild<QTableView *>(QStringLiteral("fileList"));
    }
    QLineEdit *address() const {
        return window_->findChild<QLineEdit *>(QStringLiteral("address"));
    }
    QLineEdit *search() const {
        return window_->findChild<QLineEdit *>(QStringLiteral("search"));
    }
    bool ready() const {
        const auto *state = window_->session()->tab(window_->currentTabId());
        return state && !state->loading && window_->session()->idle();
    }
    QStringList shownNames() const {
        QStringList result;
        for (int row = 0; row < table()->model()->rowCount(); ++row)
            result.push_back(table()->model()->data(table()->model()->index(row, 0)).toString());
        return result;
    }
    QStringList expectedNames(const QString &relative) const {
        QStringList result;
        for (const auto &value : manifest_.value(QStringLiteral("expected"))
                                     .toObject()
                                     .value(QStringLiteral("listing"))
                                     .toObject()
                                     .value(relative)
                                     .toArray())
            result.push_back(value.toString());
        return result;
    }
  private slots:
    void initTestCase() {
#ifdef EXPLORER_SOURCE_DIR
        repository_ = QString::fromUtf8(EXPLORER_SOURCE_DIR);
#else
        repository_ = QString::fromUtf8(EXPLORER_PROJECT_DIR);
#endif
        fixture_ = QDir(repository_).filePath(QStringLiteral("test-data/fixture-v1"));
        QFile manifest(QDir(fixture_).filePath(QStringLiteral("manifest.json")));
        QVERIFY2(manifest.open(QIODevice::ReadOnly), qPrintable(manifest.errorString()));
        const auto document = QJsonDocument::fromJson(manifest.readAll());
        QVERIFY(document.isObject());
        manifest_ = document.object();
    }

    void init() {
        window_ = std::make_unique<MainWindow>(makePlatform());
        window_->show();
        window_->raise();
        window_->activateWindow();
        QVERIFY(QTest::qWaitFor([this] { return ready(); }, 10000));
        QVERIFY(table());
        QVERIFY(address());
        QVERIFY(search());
    }

    void cleanup() {
        if (!window_)
            return;
        window_->close();
        QVERIFY2(QTest::qWaitFor([this] { return !window_->isVisible(); }, 10000),
                 "Window must finish asynchronous shutdown before destruction.");
        QVERIFY(window_->session()->idle());
        window_.reset();
    }

    void layoutAndKeyboardAddressOpenEmptyFolder() {
        QVERIFY(window_->findChild<QTabBar *>(QStringLiteral("tabs")));
        QVERIFY(window_->findChild<QTreeWidget *>(QStringLiteral("navigationTree")));
        QVERIFY(window_->findChild<QLabel *>(QStringLiteral("status")));
        for (const auto name : {"back", "forward", "up", "refresh", "newTab"})
            QVERIFY(window_->findChild<QToolButton *>(QString::fromLatin1(name)));
        table()->setFocus();
        QTest::keyClick(table(), Qt::Key_D, Qt::AltModifier);
        QTRY_VERIFY_WITH_TIMEOUT(address()->hasFocus(), 3000);
        address()->setText(path(QStringLiteral("read-only/empty")));
        QTest::keyClick(address(), Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto *state = window_->session()->tab(window_->currentTabId());
        QVERIFY(!state->error);
        const auto parsed =
            window_->session()->platform()->parseLocation(path(QStringLiteral("read-only/empty")));
        QVERIFY(!failure(parsed));
        QCOMPARE(state->location.key, std::get<Location>(parsed).key);
        QCOMPARE(table()->model()->rowCount(), 0);
        QCOMPARE(table()->model()->columnCount(), 4);
        QCOMPARE(table()->model()->headerData(0, Qt::Horizontal).toString(), QStringLiteral("Name"));
        QCOMPARE(table()->model()->headerData(3, Qt::Horizontal).toString(), QStringLiteral("Size"));
        window_->resize(1000, 650);
        QCoreApplication::processEvents();
        QVERIFY(table()->width() > 200);
        QVERIFY(address()->width() >= 250);
    }

    void allBulkAndUnicodeModelNamesMatchFixture_data() {
        QTest::addColumn<QString>("relative");
        QTest::newRow("all-1200-model-rows") << QStringLiteral("read-only/bulk");
        QTest::newRow("unicode-model-rows") << QStringLiteral("read-only/unicode");
    }
    void allBulkAndUnicodeModelNamesMatchFixture() {
        QFETCH(QString, relative);
        window_->openPath(path(relative));
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QVERIFY(!window_->session()->tab(window_->currentTabId())->error);
        auto actual = shownNames();
        auto expected = expectedNames(relative);
        actual.sort(Qt::CaseSensitive);
        expected.sort(Qt::CaseSensitive);
        QCOMPARE(actual, expected);
        for (int row = 0; row < table()->model()->rowCount(); ++row) {
            const auto index = table()->model()->index(row, 0);
            QVERIFY(!table()->model()->data(index, Qt::UserRole).toString().isEmpty());
            QVERIFY(!table()->model()->data(index, Qt::UserRole + 1).toString().isEmpty());
            QVERIFY(!table()->model()->data(table()->model()->index(row, 1)).toString().isEmpty());
            QVERIFY(!table()->model()->data(table()->model()->index(row, 2)).toString().isEmpty());
            QVERIFY(!table()->model()->data(table()->model()->index(row, 3)).toString().isEmpty());
        }
    }

    void currentFolderLiteralSearch() {
        window_->openPath(path(QStringLiteral("read-only/unicode")));
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        search()->setFocus();
        search()->setText(QString::fromUtf8("한글"));
        QTRY_COMPARE(table()->model()->rowCount(), 2);
        auto matches = shownNames();
        matches.sort(Qt::CaseSensitive);
        QCOMPARE(matches, (QStringList{QString::fromUtf8("한글_특수문자_é_😀.txt"),
                                       QString::fromUtf8("한글_파일.txt")}));
        search()->setText(QStringLiteral("no-such-name"));
        QTRY_COMPARE(table()->model()->rowCount(), 0);
        search()->clear();
        QTRY_COMPARE(table()->model()->rowCount(), 6);
    }

    void fullManifestOrderForEverySortDirection_data() {
        QTest::addColumn<QString>("relative");
        QTest::newRow("all-1200-files-with-size-and-time-ties") << QStringLiteral("read-only/bulk");
        QTest::newRow("directory-before-file-in-both-directions")
            << QStringLiteral("read-only/navigation/alpha");
    }
    void fullManifestOrderForEverySortDirection() {
        QFETCH(QString, relative);
        struct Expected {
            QString name;
            bool directory = false;
            qint64 bytes = 0, modified = 0;
        };
        QVector<Expected> facts;
        const auto prefix = relative + QLatin1Char('/');
        for (const auto &file : manifest_.value(QStringLiteral("files")).toArray()) {
            const auto value = file.toObject();
            const auto name = value.value(QStringLiteral("path")).toString();
            if (!name.startsWith(prefix) || name.mid(prefix.size()).contains(QLatin1Char('/')))
                continue;
            facts.push_back({name.mid(prefix.size()), false, value.value(QStringLiteral("bytes")).toInteger(),
                             value.value(QStringLiteral("modified_epoch_seconds")).toInteger()});
        }
        for (const auto &directory : manifest_.value(QStringLiteral("directories")).toArray()) {
            const auto name = directory.toString();
            if (name.startsWith(prefix) && !name.mid(prefix.size()).contains(QLatin1Char('/')))
                facts.push_back({name.mid(prefix.size()), true, 0, 0});
        }
        if (relative.endsWith(QStringLiteral("bulk"))) {
            QCOMPARE(facts.size(), 1200);
            QSet<qint64> sizes, times;
            for (const auto &fact : facts) {
                sizes.insert(fact.bytes);
                times.insert(fact.modified);
            }
            QVERIFY(sizes.size() < facts.size());
            QVERIFY(times.size() < facts.size()); // Both metadata sorts exercise ties.
        } else
            QCOMPARE(facts.size(), 2);
        window_->openPath(path(relative));
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QVERIFY(!window_->session()->tab(window_->currentTabId())->error);

        // Manifest bulk names are zero-padded ASCII, so codepoint and numeric
        // name orders agree independently of the platform collation API. Alpha
        // has one directory plus one file: directory timestamps are not fixed
        // by the manifest and no within-directory date order is asserted.
        for (const int column : {0, 3, 1}) {
            const auto selected =
                column == 3 ? SortColumn::Size : (column == 1 ? SortColumn::Modified : SortColumn::Name);
            for (const bool ascending : {true, false}) {
                auto oracle = facts;
                std::stable_sort(oracle.begin(), oracle.end(), [=](const Expected &a, const Expected &b) {
                    if (a.directory != b.directory)
                        return a.directory;
                    if (column == 0)
                        return ascending ? a.name < b.name : b.name < a.name;
                    const qint64 left = column == 3 ? a.bytes : a.modified;
                    const qint64 right = column == 3 ? b.bytes : b.modified;
                    if (left == right)
                        return a.name < b.name; // Tie names stay ascending, even for descending primary.
                    return ascending ? left < right : right < left;
                });
                auto *header = table()->horizontalHeader();
                for (int attempts = 0; attempts < 2; ++attempts) {
                    const auto sort = window_->session()->tab(window_->currentTabId())->sort;
                    if (sort.column == selected && sort.ascending == ascending)
                        break;
                    table()->scrollTo(table()->model()->index(0, column),
                                      QAbstractItemView::PositionAtCenter);
                    const QPoint click(header->sectionViewportPosition(column) +
                                           header->sectionSize(column) / 2,
                                       header->height() / 2);
                    QVERIFY(header->viewport()->rect().contains(click));
                    QTest::mouseClick(header->viewport(), Qt::LeftButton, Qt::NoModifier, click);
                }
                const auto state = window_->session()->tab(window_->currentTabId());
                QCOMPARE(state->sort.column, selected);
                QCOMPARE(state->sort.ascending, ascending);
                QCOMPARE(header->sortIndicatorSection(), column);
                QStringList expected;
                for (const auto &item : oracle)
                    expected.push_back(item.name);
                QCOMPARE(shownNames(), expected); // Full ordered list for each of six choices.
            }
        }
    }

    void keyboardTabsKeepIndependentPathsAndRecoverAfterInvalidInput() {
        window_->openPath(path(QStringLiteral("read-only/empty")));
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto first = window_->currentTabId();
        const auto firstPath = window_->session()->tab(first)->location.path;
        table()->setFocus();
        QTest::keyClick(table(), Qt::Key_T, Qt::ControlModifier);
        QTRY_COMPARE(window_->session()->tabs().size(), 2);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto second = window_->currentTabId();
        QVERIFY(first != second);
        window_->openPath(path(QStringLiteral("read-only/unicode")));
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto committed = window_->session()->tab(second)->location;
        const auto cursor = window_->session()->tab(second)->history.cursor();
        QCOMPARE(window_->session()->tab(first)->location.path, firstPath);
        table()->setFocus();
        QTest::keyClick(table(), Qt::Key_D, Qt::AltModifier);
        QTRY_VERIFY_WITH_TIMEOUT(address()->hasFocus(), 3000);
        address()->setText(path(QStringLiteral("read-only/errors/does-not-exist")));
        QTest::keyClick(address(), Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QCOMPARE(window_->session()->tab(second)->location.key, committed.key);
        QCOMPARE(window_->session()->tab(second)->history.cursor(), cursor);
        QCOMPARE(table()->model()->rowCount(), 6);
        QVERIFY(window_->findChild<QLabel *>(QStringLiteral("errorBanner"))->isVisible());
        auto *tabs = window_->findChild<QTabBar *>(QStringLiteral("tabs"));
        QTest::mouseClick(tabs, Qt::LeftButton, Qt::NoModifier, tabs->tabRect(0).center());
        QCOMPARE(window_->currentTabId(), first);
        QCOMPARE(table()->model()->rowCount(), 0);
        QTest::mouseClick(tabs, Qt::LeftButton, Qt::NoModifier, tabs->tabRect(1).center());
        QCOMPARE(window_->currentTabId(), second);
        QCOMPARE(table()->model()->rowCount(), 6);
        table()->setFocus();
        QTest::keyClick(table(), Qt::Key_W, Qt::ControlModifier);
        QTRY_COMPARE(window_->session()->tabs().size(), 1);
        QCOMPARE(window_->currentTabId(), first);
        QCOMPARE(window_->session()->tab(first)->location.path, firstPath);
        QCOMPARE(table()->model()->rowCount(), 0);
    }

    void closingInactiveTabKeepsSelectedTabWithThreeTabs() {
        window_->openPath(path(QStringLiteral("read-only/empty")));
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto first = window_->currentTabId();
        table()->setFocus();
        QTest::keyClick(table(), Qt::Key_T, Qt::ControlModifier);
        QTRY_COMPARE(window_->session()->tabs().size(), 2);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        window_->openPath(path(QStringLiteral("read-only/unicode")));
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto selected = window_->currentTabId();
        const auto selectedKey = window_->session()->tab(selected)->location.key;
        table()->setFocus();
        QTest::keyClick(table(), Qt::Key_T, Qt::ControlModifier);
        QTRY_COMPARE(window_->session()->tabs().size(), 3);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto third = window_->currentTabId();
        QVERIFY(first != selected && selected != third);

        auto *tabs = window_->findChild<QTabBar *>(QStringLiteral("tabs"));
        QTest::mouseClick(tabs, Qt::LeftButton, Qt::NoModifier, tabs->tabRect(1).center());
        QCOMPARE(window_->currentTabId(), selected);
        QCOMPARE(table()->model()->rowCount(), 6);
        QWidget *close = tabs->tabButton(0, QTabBar::RightSide);
        if (!close)
            close = tabs->tabButton(0, QTabBar::LeftSide);
        QVERIFY2(close, "A closable inactive tab must expose its Qt close button.");
        QTest::mouseClick(close, Qt::LeftButton);
        QTRY_COMPARE(window_->session()->tabs().size(), 2);
        QVERIFY(!window_->session()->tab(first));
        QCOMPARE(window_->currentTabId(), selected);
        QCOMPARE(window_->session()->tab(selected)->location.key, selectedKey);
        QCOMPARE(table()->model()->rowCount(), 6);
        QVERIFY(window_->session()->tab(third));
    }

    void realDirectoryDoubleClickUsesVisibleModelEntry() {
        window_->openPath(path(QStringLiteral("read-only/navigation")));
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QModelIndex alpha;
        for (int row = 0; row < table()->model()->rowCount(); ++row) {
            const auto index = table()->model()->index(row, 0);
            if (index.data().toString() == QStringLiteral("alpha"))
                alpha = index;
        }
        QVERIFY(alpha.isValid());
        table()->scrollTo(alpha);
        const auto position = table()->visualRect(alpha).center();
        QTest::mouseClick(table()->viewport(), Qt::LeftButton, Qt::NoModifier, position);
        QTest::mouseDClick(table()->viewport(), Qt::LeftButton, Qt::NoModifier, position);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto parsed =
            window_->session()->platform()->parseLocation(path(QStringLiteral("read-only/navigation/alpha")));
        QVERIFY(!failure(parsed));
        QCOMPARE(window_->session()->tab(window_->currentTabId())->location.key,
                 std::get<Location>(parsed).key);
        QVERIFY(shownNames().contains(QStringLiteral("beta")));
    }

    void navigationButtonsBackForwardAndUp() {
        window_->openPath(path(QStringLiteral("read-only/navigation/alpha")));
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto alpha = window_->session()->tab(window_->currentTabId())->location;
        window_->openPath(path(QStringLiteral("read-only/navigation/alpha/beta")));
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto beta = window_->session()->tab(window_->currentTabId())->location;
        QTest::mouseClick(window_->findChild<QToolButton *>(QStringLiteral("back")), Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QCOMPARE(window_->session()->tab(window_->currentTabId())->location.key, alpha.key);
        QTest::mouseClick(window_->findChild<QToolButton *>(QStringLiteral("forward")), Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QCOMPARE(window_->session()->tab(window_->currentTabId())->location.key, beta.key);
        QTest::mouseClick(window_->findChild<QToolButton *>(QStringLiteral("up")), Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QCOMPARE(window_->session()->tab(window_->currentTabId())->location.key, alpha.key);
    }
};

QTEST_MAIN(UiTests)
#include "ui_tests.moc"
