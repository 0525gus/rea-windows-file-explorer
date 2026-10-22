#include "explorer/main_window.hpp"
#include "operation_sandbox.hpp"
#include <QAbstractItemModel>
#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFile>
#include <QFontMetrics>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMap>
#include <QMimeData>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QTabBar>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QUrl>
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
    QModelIndex namedIndex(const QString &name) const {
        for (int row = 0; row < table()->model()->rowCount(); ++row) {
            const auto index = table()->model()->index(row, 0);
            if (index.data().toString() == name)
                return index;
        }
        return {};
    }
    bool focusFiles() {
        window_->activateWindow();
        table()->setFocus();
        return QTest::qWaitFor([this] { return table()->hasFocus(); }, 3000);
    }
    void clickName(const QString &name, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        QVERIFY2(focusFiles(), "Restore the actual file-view focus after any modal interaction.");
        const auto index = namedIndex(name);
        QVERIFY2(index.isValid(), qPrintable(QStringLiteral("Missing UI item: ") + name));
        table()->scrollTo(index);
        const auto position = table()->visualRect(index).center();
        QVERIFY(table()->viewport()->rect().contains(position));
        QTest::mouseClick(table()->viewport(), Qt::LeftButton, modifiers, position);
    }
    static bool writeNew(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly | QIODevice::NewOnly) && file.write(bytes) == bytes.size() &&
               file.flush();
    }
    static QByteArray fileHash(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!hash.addData(&file))
            return {};
        return hash.result();
    }
    bool enterInputDialog(const QString &text, const std::function<void()> &trigger) {
        bool entered = false;
        QElapsedTimer elapsed;
        elapsed.start();
        QTimer responder;
        responder.setInterval(10);
        connect(&responder, &QTimer::timeout, &responder, [&] {
            if (auto *input = qobject_cast<QInputDialog *>(QApplication::activeModalWidget())) {
                entered = true;
                input->setTextValue(text);
                input->accept();
                responder.stop();
            } else if (elapsed.elapsed() > 2000) {
                if (auto *unexpected = qobject_cast<QDialog *>(QApplication::activeModalWidget()))
                    unexpected->reject();
                responder.stop();
            }
        });
        responder.start();
        trigger();
        return entered;
    }
  private slots:
    void initTestCase() {
        QVERIFY2(!operationSandboxTemplate(QStringLiteral("runtime-ui-operations")).isEmpty(),
                 "REA_OPERATION_TEST_ROOT must name an existing absolute directory.");
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

    void breadcrumbAncestorClickCommitsHistoryAndCtrlLEditsCompletePath() {
        QTemporaryDir sandbox(operationSandboxTemplate(QStringLiteral("runtime-ui-crumb")));
        QVERIFY(sandbox.isValid());
        const auto parentPath = sandbox.filePath(QStringLiteral("alpha"));
        const auto childPath = sandbox.filePath(QStringLiteral("alpha/beta"));
        QVERIFY(QDir().mkpath(childPath));
        QVERIFY(writeNew(QDir(parentPath).filePath(QStringLiteral("parent-only.txt")), QByteArray("parent")));
        QVERIFY(writeNew(QDir(childPath).filePath(QStringLiteral("child-only.txt")), QByteArray("child")));
        // Keep every label visible so the test need not reinterpret the widget's
        // elision/hit map. Click geometry uses only public font metrics and the
        // known complete fixture path; no private AddressEdit API is exposed.
        window_->resize(2400, 800);
        window_->openPath(childPath);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QCOMPARE(shownNames(), QStringList{QStringLiteral("child-only.txt")});
        const auto id = window_->currentTabId();
        const auto child = window_->session()->tab(id)->location;
        const auto beforeCursor = window_->session()->tab(id)->history.cursor();
        const auto beforeCount = window_->session()->tab(id)->history.locations().size();
        const auto parent = window_->session()->platform()->parseLocation(parentPath);
        QVERIFY(!failure(parent));
        QCOMPARE(address()->text(), child.path);
        QVERIFY(focusFiles());
        QVERIFY(!address()->hasFocus());
        address()->repaint(); // Establish the real unfocused breadcrumb paint before clicking.
        QStringList labels;
        QDir directory(childPath);
        while (true) {
            labels.prepend(directory.isRoot() ? QDir::toNativeSeparators(directory.absolutePath())
                                              : directory.dirName());
            if (directory.isRoot())
                break;
            QVERIFY(directory.cdUp());
        }
        QVERIFY(labels.size() >= 3);
        QCOMPARE(labels[labels.size() - 2], QStringLiteral("alpha"));
        QCOMPARE(labels.last(), QStringLiteral("beta"));
        const QFontMetrics metrics(address()->font());
        int x = 42;
        int completeWidth = x;
        for (const auto &label : labels)
            completeWidth += metrics.horizontalAdvance(label) + 30;
        QVERIFY2(address()->width() > completeWidth + 24,
                 "All breadcrumb labels must fit; this test does not assume an elision policy.");
        for (qsizetype i = 0; i + 2 < labels.size(); ++i)
            x += metrics.horizontalAdvance(labels[i]) + 30;
        const QPoint ancestor(x + metrics.horizontalAdvance(QStringLiteral("alpha")) / 2,
                              address()->height() / 2);
        QVERIFY(address()->rect().contains(ancestor));
        QTest::mouseClick(address(), Qt::LeftButton, Qt::NoModifier, ancestor);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QCOMPARE(window_->session()->tab(id)->location.key, std::get<Location>(parent).key);
        QCOMPARE(shownNames(), (QStringList{QStringLiteral("beta"), QStringLiteral("parent-only.txt")}));
        QCOMPARE(window_->session()->tab(id)->history.cursor(), beforeCursor + 1);
        QCOMPARE(window_->session()->tab(id)->history.locations().size(), beforeCount + 1);
        QCOMPARE(window_->session()->tab(id)->history.at(beforeCursor)->key, child.key);
        QVERIFY(focusFiles());
        QTest::keyClick(table(), Qt::Key_Left, Qt::AltModifier);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QCOMPARE(window_->session()->tab(id)->location.key, child.key);
        QCOMPARE(window_->session()->tab(id)->history.cursor(), beforeCursor);
        QCOMPARE(shownNames(), QStringList{QStringLiteral("child-only.txt")});
        QTest::keyClick(table(), Qt::Key_L, Qt::ControlModifier);
        QTRY_VERIFY_WITH_TIMEOUT(address()->hasFocus(), 3000);
        QCOMPARE(address()->text(), child.path);
        QCOMPARE(address()->selectedText(), child.path);
        const auto missing = QDir(childPath).filePath(QStringLiteral("missing-descendant"));
        address()->setText(missing);
        QTest::keyClick(address(), Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QVERIFY(window_->session()->tab(id)->error);
        QCOMPARE(window_->session()->tab(id)->error->code, ErrorCode::NotFound);
        QCOMPARE(window_->session()->tab(id)->location.key, child.key);
        QCOMPARE(window_->session()->tab(id)->history.cursor(), beforeCursor);
        QCOMPARE(shownNames(), QStringList{QStringLiteral("child-only.txt")});
        QCOMPARE(address()->text(), missing);
        QVERIFY(window_->findChild<QLabel *>(QStringLiteral("errorBanner"))->isVisible());
    }

    void realKeyboardMultiSelectionCopyCutPastePreservesBytes() {
        QTemporaryDir sandbox(operationSandboxTemplate(QStringLiteral("runtime-ui-transfer")));
        QVERIFY(sandbox.isValid());
        const auto source = sandbox.filePath(QStringLiteral("source"));
        const auto copied = sandbox.filePath(QStringLiteral("copied"));
        const auto moved = sandbox.filePath(QStringLiteral("moved"));
        QVERIFY(QDir().mkpath(source) && QDir().mkpath(copied) && QDir().mkpath(moved));
        const QStringList names{QStringLiteral("a.txt"), QStringLiteral("b.txt"),
                                QString::fromUtf8("c_한글.txt")};
        QMap<QString, QByteArray> hashes;
        for (const auto &name : names) {
            const auto file = QDir(source).filePath(name);
            QVERIFY(writeNew(file, QByteArray("original bytes:") + name.toUtf8()));
            hashes.insert(name, fileHash(file));
        }
        window_->openPath(source);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        clickName(names[0]);
        clickName(names[2], Qt::ControlModifier);
        QCOMPARE(table()->selectionModel()->selectedRows().size(), 2);
        QCOMPARE(window_->session()->tab(window_->currentTabId())->selection.size(), 2);
        QVERIFY(window_->findChild<QLabel *>(QStringLiteral("status"))
                    ->text()
                    .contains(QStringLiteral("2 selected")));
        QTest::keyClick(table(), Qt::Key_C, Qt::ControlModifier);
        const auto *mime = QApplication::clipboard()->mimeData();
        QVERIFY(mime && mime->hasUrls());
        QSet<QString> urls;
        for (const auto &url : mime->urls())
            urls.insert(QFileInfo(url.toLocalFile()).absoluteFilePath());
        QCOMPARE(urls, (QSet<QString>{QDir(source).filePath(names[0]), QDir(source).filePath(names[2])}));
        window_->openPath(copied);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        table()->setFocus();
        QTest::keyClick(table(), Qt::Key_V, Qt::ControlModifier);
        QTRY_VERIFY_WITH_TIMEOUT(window_->session()->operationState().report.has_value() && ready(), 10000);
        auto report = *window_->session()->operationState().report;
        QCOMPARE(report.kind, OperationKind::Copy);
        QCOMPARE(report.items.size(), 2);
        for (const auto &outcome : report.items)
            QCOMPARE(outcome.status, ItemStatus::Succeeded);
        QCOMPARE(shownNames(), (QStringList{names[0], names[2]}));
        for (const auto &name : names)
            QCOMPARE(fileHash(QDir(source).filePath(name)), hashes.value(name));
        for (const auto &name : {names[0], names[2]})
            QCOMPARE(fileHash(QDir(copied).filePath(name)), hashes.value(name));
        QVERIFY(!QFileInfo::exists(QDir(copied).filePath(names[1])));
        auto *operationStatus = window_->findChild<QLabel *>(QStringLiteral("operationStatus"));
        QVERIFY(operationStatus);
        QTRY_VERIFY(operationStatus->text().contains(QStringLiteral("2 succeeded")));
        // A second real paste must report both conflicts, then retry only those
        // through the visible Keep both command without replacing the originals.
        QTest::keyClick(table(), Qt::Key_V, Qt::ControlModifier);
        QTRY_VERIFY_WITH_TIMEOUT(window_->session()->operationState().report.has_value() && ready(), 10000);
        report = *window_->session()->operationState().report;
        QCOMPARE(report.items.size(), 2);
        for (const auto &outcome : report.items) {
            QCOMPARE(outcome.status, ItemStatus::Failed);
            QVERIFY(outcome.error && outcome.error->code == ErrorCode::Conflict);
        }
        auto *keepBoth = window_->findChild<QToolButton *>(QStringLiteral("keepBoth"));
        QVERIFY(keepBoth);
        QTRY_VERIFY(keepBoth->isVisible() && keepBoth->isEnabled());
        QTest::mouseClick(keepBoth, Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(window_->session()->operationState().report.has_value() && ready(), 10000);
        report = *window_->session()->operationState().report;
        QCOMPARE(report.items.size(), 2);
        for (const auto &outcome : report.items) {
            QCOMPARE(outcome.status, ItemStatus::Succeeded);
            const auto sourceName = QFileInfo(outcome.sourcePath).fileName();
            QVERIFY(names.contains(sourceName));
            QVERIFY(QFileInfo(outcome.destinationPath).absoluteFilePath() !=
                    QDir(copied).filePath(sourceName));
            QCOMPARE(fileHash(outcome.destinationPath), hashes.value(sourceName));
        }
        QCOMPARE(shownNames().size(), 4);
        for (const auto &name : {names[0], names[2]})
            QCOMPARE(fileHash(QDir(copied).filePath(name)), hashes.value(name));

        window_->openPath(source);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        clickName(names[0]);
        clickName(names[2], Qt::ShiftModifier);
        QCOMPARE(table()->selectionModel()->selectedRows().size(), 3);
        QTest::keyClick(table(), Qt::Key_X, Qt::ControlModifier);
        window_->openPath(moved);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        table()->setFocus();
        QTest::keyClick(table(), Qt::Key_V, Qt::ControlModifier);
        QTRY_VERIFY_WITH_TIMEOUT(
            window_->session()->operationState().report.has_value() &&
                window_->session()->operationState().report->kind == OperationKind::Move && ready(),
            10000);
        report = *window_->session()->operationState().report;
        QCOMPARE(report.items.size(), 3);
        for (const auto &outcome : report.items)
            QCOMPARE(outcome.status, ItemStatus::Succeeded);
        QCOMPARE(shownNames(), names);
        for (const auto &name : names) {
            QVERIFY(!QFileInfo::exists(QDir(source).filePath(name)));
            QCOMPARE(fileHash(QDir(moved).filePath(name)), hashes.value(name));
        }
        for (const auto &name : {names[0], names[2]})
            QCOMPARE(fileHash(QDir(copied).filePath(name)), hashes.value(name));
        QVERIFY(QDir(source).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty());
    }

    void keyboardRenameAndNewDirectoryUseRealDialogs() {
        QTemporaryDir sandbox(operationSandboxTemplate(QStringLiteral("runtime-ui-names")));
        QVERIFY(sandbox.isValid());
        const auto original = sandbox.filePath(QStringLiteral("original.txt"));
        QVERIFY(writeNew(original, QByteArray("keep through UI rename")));
        const auto before = fileHash(original);
        window_->openPath(sandbox.path());
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        clickName(QStringLiteral("original.txt"));
        const auto renamed = QString::fromUtf8("새 파일 이름.txt");
        QVERIFY(enterInputDialog(renamed, [this] { QTest::keyClick(table(), Qt::Key_F2); }));
        QTRY_VERIFY_WITH_TIMEOUT(window_->session()->operationState().report.has_value() && ready(), 10000);
        QCOMPARE(window_->session()->operationState().report->kind, OperationKind::Rename);
        QCOMPARE(window_->session()->operationState().report->items.first().status, ItemStatus::Succeeded);
        QVERIFY(!QFileInfo::exists(original));
        QCOMPARE(fileHash(sandbox.filePath(renamed)), before);
        const auto folder = QString::fromUtf8("새 폴더 한글");
        QVERIFY(focusFiles());
        QVERIFY(enterInputDialog(folder, [this] {
            QTest::keyClick(table(), Qt::Key_N, Qt::ControlModifier | Qt::ShiftModifier);
        }));
        QTRY_VERIFY_WITH_TIMEOUT(window_->session()->operationState().report.has_value() &&
                                     window_->session()->operationState().report->kind ==
                                         OperationKind::CreateDirectory &&
                                     ready(),
                                 10000);
        QCOMPARE(window_->session()->operationState().report->items.first().status, ItemStatus::Succeeded);
        QVERIFY(QFileInfo(sandbox.filePath(folder)).isDir());
        clickName(folder);
        const auto newFolderName = QString::fromUtf8("이름 바꾼 폴더");
        QVERIFY(enterInputDialog(newFolderName, [this] { QTest::keyClick(table(), Qt::Key_F2); }));
        QTRY_VERIFY_WITH_TIMEOUT(
            window_->session()->operationState().report.has_value() &&
                window_->session()->operationState().report->kind == OperationKind::Rename && ready(),
            10000);
        QCOMPARE(window_->session()->operationState().report->items.first().status, ItemStatus::Succeeded);
        QVERIFY(!QFileInfo::exists(sandbox.filePath(folder)));
        QVERIFY(QFileInfo(sandbox.filePath(newFolderName)).isDir());
        QCOMPARE(fileHash(sandbox.filePath(renamed)), before);
        QVERIFY(shownNames().contains(newFolderName) && shownNames().contains(renamed));
    }

    void keyboardViewModesKeepItemsSelectionAndTabMode() {
        window_->openPath(path(QStringLiteral("read-only/unicode")));
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto first = window_->currentTabId();
        auto names = shownNames();
        clickName(QString::fromUtf8("공백 있는 이름.txt"));
        if (QTest::currentTestFailed())
            return;
        const auto selected = window_->session()->tab(first)->selection;
        auto *icons = window_->findChild<QListView *>(QStringLiteral("iconList"));
        auto *views = window_->findChild<QStackedWidget *>(QStringLiteral("fileViews"));
        QVERIFY(icons && views);
        QTest::keyClick(table(), Qt::Key_1, Qt::ControlModifier | Qt::ShiftModifier);
        QTRY_COMPARE(views->currentWidget(), static_cast<QWidget *>(icons));
        QCOMPARE(icons->viewMode(), QListView::IconMode);
        QCOMPARE(icons->model()->rowCount(), names.size());
        QCOMPARE(window_->session()->tab(first)->selection, selected);
        QCOMPARE(icons->selectionModel()->selectedRows().size(), 1);
        QTest::keyClick(icons, Qt::Key_5, Qt::ControlModifier | Qt::ShiftModifier);
        QCOMPARE(icons->viewMode(), QListView::ListMode);
        QCOMPARE(shownNames(), names);
        QCOMPARE(window_->session()->tab(first)->selection, selected);
        QTest::keyClick(icons, Qt::Key_T, Qt::ControlModifier);
        QTRY_COMPARE(window_->session()->tabs().size(), 2);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QCOMPARE(views->currentWidget(), static_cast<QWidget *>(table()));
        auto *tabs = window_->findChild<QTabBar *>(QStringLiteral("tabs"));
        QTest::mouseClick(tabs, Qt::LeftButton, Qt::NoModifier, tabs->tabRect(0).center());
        QCOMPARE(window_->currentTabId(), first);
        QCOMPARE(views->currentWidget(), static_cast<QWidget *>(icons));
        QCOMPARE(icons->viewMode(), QListView::ListMode);
        QCOMPARE(shownNames(), names);
        QTest::keyClick(icons, Qt::Key_6, Qt::ControlModifier | Qt::ShiftModifier);
        QCOMPARE(views->currentWidget(), static_cast<QWidget *>(table()));
        QCOMPARE(table()->model()->columnCount(), 4);
        QCOMPARE(window_->session()->tab(first)->selection, selected);
    }

    void externalFileUrlDropCopiesOrMovesIntoVisibleFolder_data() {
        QTest::addColumn<int>("sourceActions");
        QTest::addColumn<int>("requestedAction");
        QTest::addColumn<bool>("accepted");
        QTest::addColumn<int>("acknowledgement");
        QTest::newRow("copy-local-file-URL")
            << int(Qt::CopyAction | Qt::MoveAction) << int(Qt::CopyAction) << true << int(Qt::CopyAction);
        QTest::newRow("move-local-file-URL")
            << int(Qt::CopyAction | Qt::MoveAction) << int(Qt::MoveAction) << true << int(Qt::CopyAction);
#ifdef Q_OS_WIN
        QTest::newRow("move-only-source-target-owns-deletion")
            << int(Qt::MoveAction) << int(Qt::MoveAction) << true << int(Qt::TargetMoveAction);
#else
        QTest::newRow("move-only-source-explicitly-refused-on-non-Windows")
            << int(Qt::MoveAction) << int(Qt::MoveAction) << false << int(Qt::IgnoreAction);
#endif
        QTest::newRow("link-only-source-refused")
            << int(Qt::LinkAction) << int(Qt::LinkAction) << false << int(Qt::IgnoreAction);
    }
    void externalFileUrlDropCopiesOrMovesIntoVisibleFolder() {
        QFETCH(int, sourceActions);
        QFETCH(int, requestedAction);
        QFETCH(bool, accepted);
        QFETCH(int, acknowledgement);
        QTemporaryDir sandbox(operationSandboxTemplate(QStringLiteral("runtime-ui-drop")));
        QVERIFY(sandbox.isValid());
        const auto source = sandbox.filePath(QStringLiteral("external.txt"));
        const auto destination = sandbox.filePath(QStringLiteral("destination"));
        const auto nested = QDir(destination).filePath(QStringLiteral("drop-here"));
        QVERIFY(writeNew(source, QByteArray("external drag payload")));
        QVERIFY(QDir().mkpath(nested));
        const auto before = fileHash(source);
        window_->openPath(destination);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto index = namedIndex(QStringLiteral("drop-here"));
        QVERIFY(index.isValid());
        table()->scrollTo(index);
        const auto position = table()->visualRect(index).center();
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(source)});
        const auto action = Qt::DropAction(requestedAction);
        const bool move = action == Qt::MoveAction;
        const auto modifiers = move ? Qt::ShiftModifier : Qt::ControlModifier;
        // Preserve the sender's advertised choices, including Move-only and
        // Link-only sources. Acceptance must not request a second source delete.
        const auto allowed = Qt::DropActions(sourceActions);
        QSignalSpy operations(window_->session(), &ExplorerSession::operationChanged);
        QDragEnterEvent enter(position, allowed, &mime, Qt::LeftButton, modifiers);
        enter.setDropAction(action);
        QApplication::sendEvent(table()->viewport(), &enter);
        if (accepted)
            QVERIFY(enter.isAccepted());
        QDragMoveEvent motion(position, allowed, &mime, Qt::LeftButton, modifiers);
        motion.setDropAction(action);
        QApplication::sendEvent(table()->viewport(), &motion);
        if (accepted)
            QVERIFY(motion.isAccepted());
        QDropEvent drop(QPointF(position), allowed, &mime, Qt::LeftButton, modifiers);
        drop.setDropAction(action);
        QApplication::sendEvent(table()->viewport(), &drop);
        QCOMPARE(drop.isAccepted(), accepted);
        if (!accepted) {
            // Rejection cannot be turned into a delayed copy or move job.
            QTest::qWait(100);
            QCOMPARE(operations.count(), 0);
            QVERIFY(!window_->session()->operationState().report);
            QVERIFY(window_->session()->idle());
            QCOMPARE(fileHash(source), before);
            QVERIFY(QDir(nested).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty());
            QCOMPARE(shownNames(), QStringList{QStringLiteral("drop-here")});
            return;
        }
        // The receiving asynchronous job owns moves; the sender must not delete twice.
        QCOMPARE(drop.dropAction(), Qt::DropAction(acknowledgement));
        QTRY_VERIFY_WITH_TIMEOUT(window_->session()->operationState().report.has_value() && ready(), 10000);
        const auto report = *window_->session()->operationState().report;
        QCOMPARE(report.kind, move ? OperationKind::Move : OperationKind::Copy);
        QCOMPARE(report.items.size(), 1);
        QCOMPARE(report.items.first().status, ItemStatus::Succeeded);
        QCOMPARE(fileHash(QDir(nested).filePath(QStringLiteral("external.txt"))), before);
        QCOMPARE(QFileInfo::exists(source), !move);
        if (!move)
            QCOMPARE(fileHash(source), before);
        QVERIFY(!QFileInfo::exists(QDir(destination).filePath(QStringLiteral("external.txt"))));
        QCOMPARE(shownNames(), QStringList{QStringLiteral("drop-here")});
    }

    void displayedFileDeletedExternallyReportsNotFoundAndRefreshRecovers() {
        QTemporaryDir sandbox(operationSandboxTemplate(QStringLiteral("runtime-ui-deleted")));
        QVERIFY(sandbox.isValid());
        const auto name = QStringLiteral("externally-removed.rea-validation");
        const auto ownedFile = sandbox.filePath(name);
        QVERIFY(writeNew(ownedFile, QByteArray("only this disposable file is removed")));
        window_->openPath(sandbox.path());
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        clickName(name);
        QCOMPARE(table()->selectionModel()->selectedRows().size(), 1);
        const auto committed = window_->session()->tab(window_->currentTabId())->location.key;
        QVERIFY(QFile::remove(ownedFile)); // External actor relative to the displayed model.
        QTest::keyClick(table(), Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto *state = window_->session()->tab(window_->currentTabId());
        QVERIFY(state && state->error);
        QCOMPARE(state->error->code, ErrorCode::NotFound);
        QCOMPARE(state->location.key, committed);
        QCOMPARE(shownNames(), QStringList{name}); // Last committed listing stays available.
        auto *banner = window_->findChild<QLabel *>(QStringLiteral("errorBanner"));
        QVERIFY(banner && banner->isVisible());
        QVERIFY(!banner->text().isEmpty());
        QVERIFY(window_->isVisible() && window_->isEnabled());
        QVERIFY(focusFiles());
        QTest::keyClick(table(), Qt::Key_F5);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QCOMPARE(shownNames().size(), 0);
        QVERIFY(!window_->session()->tab(window_->currentTabId())->error);
        QVERIFY(!banner->isVisible());
        QVERIFY(window_->findChild<QLabel *>(QStringLiteral("status"))
                    ->text()
                    .startsWith(QStringLiteral("0 items")));
        QVERIFY(!QFileInfo::exists(ownedFile));
    }

    void folderRenameCollisionPreservesBothTreesAndValidRetryRecovers() {
        QTemporaryDir sandbox(operationSandboxTemplate(QStringLiteral("runtime-ui-folder-conflict")));
        QVERIFY(sandbox.isValid());
        const auto original = sandbox.filePath(QStringLiteral("original-folder"));
        const auto occupied = sandbox.filePath(QStringLiteral("occupied-folder"));
        QVERIFY(QDir().mkpath(QDir(original).filePath(QStringLiteral("nested"))));
        QVERIFY(QDir().mkpath(QDir(occupied).filePath(QStringLiteral("nested"))));
        const auto sourceLeaf = QDir(original).filePath(QStringLiteral("nested/source.txt"));
        const auto occupiedLeaf = QDir(occupied).filePath(QStringLiteral("nested/existing.txt"));
        QVERIFY(writeNew(sourceLeaf, QByteArray("source subtree must survive collision")));
        QVERIFY(writeNew(occupiedLeaf, QByteArray("occupied subtree must never be replaced")));
        const auto sourceHash = fileHash(sourceLeaf), occupiedHash = fileHash(occupiedLeaf);
        window_->openPath(sandbox.path());
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        clickName(QStringLiteral("original-folder"));
        QVERIFY(enterInputDialog(QStringLiteral("occupied-folder"),
                                 [this] { QTest::keyClick(table(), Qt::Key_F2); }));
        QTRY_VERIFY_WITH_TIMEOUT(window_->session()->operationState().report.has_value() && ready(), 10000);
        auto report = *window_->session()->operationState().report;
        QCOMPARE(report.kind, OperationKind::Rename);
        QCOMPARE(report.items.size(), 1);
        QCOMPARE(report.items.first().status, ItemStatus::Failed);
        QVERIFY(report.items.first().error);
        QCOMPARE(report.items.first().error->code, ErrorCode::Conflict);
        QCOMPARE(fileHash(sourceLeaf), sourceHash);
        QCOMPARE(fileHash(occupiedLeaf), occupiedHash);
        QCOMPARE(shownNames(),
                 (QStringList{QStringLiteral("occupied-folder"), QStringLiteral("original-folder")}));
        auto *status = window_->findChild<QLabel *>(QStringLiteral("operationStatus"));
        QVERIFY(status && status->isVisible());
        QVERIFY(status->text().contains(QStringLiteral("1 failed/partial")));
        QVERIFY(status->toolTip().contains(QStringLiteral("original-folder")));
        clickName(QStringLiteral("original-folder"));
        const auto validName = QString::fromUtf8("복구된 폴더");
        QVERIFY(enterInputDialog(validName, [this] { QTest::keyClick(table(), Qt::Key_F2); }));
        QTRY_VERIFY_WITH_TIMEOUT(window_->session()->operationState().report.has_value() && ready(), 10000);
        report = *window_->session()->operationState().report;
        QCOMPARE(report.items.size(), 1);
        QCOMPARE(report.items.first().status, ItemStatus::Succeeded);
        QVERIFY(!QFileInfo::exists(original));
        QCOMPARE(fileHash(sandbox.filePath(validName + QStringLiteral("/nested/source.txt"))), sourceHash);
        QCOMPARE(fileHash(occupiedLeaf), occupiedHash);
        QVERIFY(!QFileInfo::exists(QDir(occupied).filePath(QStringLiteral("nested/source.txt"))));
        QVERIFY(shownNames().contains(validName));
    }

    void closeDuringProductionCopySettlesOwnershipAndFreshWindowReopens() {
        QTemporaryDir sandbox(operationSandboxTemplate(QStringLiteral("runtime-ui-shutdown-copy")));
        QVERIFY(sandbox.isValid());
        const auto sourceDirectory = sandbox.filePath(QStringLiteral("source"));
        const auto destinationDirectory = sandbox.filePath(QStringLiteral("destination"));
        QVERIFY(QDir().mkpath(sourceDirectory) && QDir().mkpath(destinationDirectory));
        const auto sourcePath = QDir(sourceDirectory).filePath(QStringLiteral("shutdown-source.bin"));
        const auto neighbor = QDir(sourceDirectory).filePath(QStringLiteral("neighbor.txt"));
        QVERIFY(writeNew(sourcePath, QByteArray(256 * 1024, 'S')));
        QVERIFY(writeNew(neighbor, QByteArray("never selected for the closing operation")));
        const auto sourceHash = fileHash(sourcePath), neighborHash = fileHash(neighbor);
        window_->openPath(sourceDirectory);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        const auto platform = window_->session()->platform();
        const auto entry = platform->inspectEntry(sourcePath);
        const auto destination = platform->parseLocation(destinationDirectory);
        QVERIFY(!failure(entry) && !failure(destination));
        QSignalSpy shutdown(window_->session(), &ExplorerSession::shutdownFinished);
        QVERIFY(window_->session()->beginOperation(
            {OperationKind::Copy, {std::get<Entry>(entry)}, std::get<Location>(destination)}));
        QVERIFY(window_->session()->operationState().running);
        window_->close(); // Production close owns cancellation and waits for actual worker completion.
        QTRY_VERIFY_WITH_TIMEOUT(!window_->isVisible(), 10000);
        QVERIFY(window_->session()->idle());
        QCOMPARE(shutdown.count(), 1);
        const auto state = window_->session()->operationState();
        QVERIFY(!state.running && state.report);
        QCOMPARE(state.report->items.size(), 1);
        const auto &outcome = state.report->items.first();
        QVERIFY(outcome.status == ItemStatus::Cancelled || outcome.status == ItemStatus::Succeeded);
        QVERIFY(outcome.retainedPaths.isEmpty() && state.report->cleanupPending.isEmpty());
        QCOMPARE(fileHash(sourcePath), sourceHash);
        QCOMPARE(fileHash(neighbor), neighborHash);
        const auto copiedPath = QDir(destinationDirectory).filePath(QStringLiteral("shutdown-source.bin"));
        if (outcome.status == ItemStatus::Succeeded) {
            QCOMPARE(fileHash(copiedPath), sourceHash);
            QCOMPARE(
                QDir(destinationDirectory).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot),
                QStringList{QStringLiteral("shutdown-source.bin")});
        } else {
            QVERIFY(state.report->cancelled);
            QVERIFY(!QFileInfo::exists(copiedPath));
            QVERIFY(QDir(destinationDirectory)
                        .entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot)
                        .isEmpty());
        }
        qInfo().noquote() << "Production close outcome:" << int(outcome.status)
                          << "copied bytes observed:" << state.progress.completedBytes;
        // This is a fresh Qt window/session in the same test process. Native
        // process restart is a separately recorded desktop/package scenario.
        window_.reset();
        window_ = std::make_unique<MainWindow>(makePlatform());
        window_->show();
        window_->activateWindow();
        window_->openPath(sourceDirectory);
        QTRY_VERIFY_WITH_TIMEOUT(ready(), 10000);
        QVERIFY(window_->isVisible() && window_->isEnabled());
        QVERIFY(!window_->session()->tab(window_->currentTabId())->error);
        QVERIFY(!window_->session()->operationState().running &&
                !window_->session()->operationState().report);
        QCOMPARE(shownNames(),
                 (QStringList{QStringLiteral("neighbor.txt"), QStringLiteral("shutdown-source.bin")}));
        QCOMPARE(fileHash(sourcePath), sourceHash);
        QCOMPARE(fileHash(neighbor), neighborHash);
    }
};

QTEST_MAIN(UiTests)
#include "ui_tests.moc"
