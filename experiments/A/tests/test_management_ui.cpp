#include "explorer/main_window.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QInputDialog>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
using namespace explorer;
class ManagementUiTest final : public QObject {
    Q_OBJECT
    QString owned() const {
        return QCoreApplication::applicationDirPath() + "/ui-mutation-XXXXXX";
    }
    void write(const QString &path, const QByteArray &content) {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QCOMPARE(f.write(content), content.size());
    }
    void ready(MainWindow &window) {
        QTRY_VERIFY(window.controller()->current() && !window.controller()->current()->loading);
    }
    QModelIndex named(MainWindow &window, const QString &name) {
        for (int row = 0; row < window.proxy()->rowCount(); ++row)
            if (window.proxy()->index(row, 0).data().toString() == name)
                return window.proxy()->index(row, 0);
        return {};
    }
    void select(MainWindow &window, const QString &name) {
        const auto index = named(window, name);
        QVERIFY(index.isValid());
        auto *table = window.findChild<QTableView *>("fileList");
        table->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect |
                                                            QItemSelectionModel::Rows);
        table->setFocus();
        window.activateWindow();
        QCoreApplication::processEvents();
    }
    void answerName(const QString &name) {
        QTimer::singleShot(30, [name] {
            auto *dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
            if (dialog) {
                dialog->setTextValue(name);
                dialog->accept();
            }
        });
    }
    void answerMessage(QTimer &timer, const QString &label) {
        connect(&timer, &QTimer::timeout, &timer, [&timer, label] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                for (auto *button : box->buttons()) {
                    if (button->text().remove('&') == label) {
                        timer.stop();
                        button->click();
                        return;
                    }
                }
        });
        timer.start(10);
    }
  private slots:
    void createAndRenameThroughDialogs() {
        QTemporaryDir temp(owned());
        QVERIFY(temp.isValid());
        MainWindow window;
        window.show();
        window.openInitial(Location::folder(temp.path()));
        ready(window);
        answerName(QString::fromUtf8("새 폴더"));
        QTest::mouseClick(window.findChild<QToolButton *>("newFolder"), Qt::LeftButton);
        QTRY_VERIFY(QFileInfo(temp.filePath(QString::fromUtf8("새 폴더"))).isDir());
        QTRY_VERIFY(!window.operations()->busy());
        ready(window);
        select(window, QString::fromUtf8("새 폴더"));
        answerName("renamed folder");
        QTest::keyClick(window.findChild<QTableView *>("fileList"), Qt::Key_F2);
        QTRY_VERIFY(QFileInfo(temp.filePath("renamed folder")).isDir());
        QTRY_VERIFY(!window.operations()->busy());
        ready(window);
        QVERIFY(!QFileInfo::exists(temp.filePath(QString::fromUtf8("새 폴더"))));
    }
    void ctrlShiftSelectionAndViews() {
        QTemporaryDir temp(owned());
        QVERIFY(temp.isValid());
        for (int i = 0; i < 4; ++i)
            write(temp.filePath(QString::number(i) + ".txt"), "a");
        MainWindow window;
        window.show();
        window.openInitial(Location::folder(temp.path()));
        ready(window);
        auto *table = window.findChild<QTableView *>("fileList");
        const auto center = [&](int row) {
            return table->visualRect(window.proxy()->index(row, 0)).center();
        };
        QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, center(0));
        QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::ControlModifier, center(2));
        QCOMPARE(window.controller()->current()->selection.size(), 2);
        QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::ShiftModifier, center(3));
        QVERIFY(window.controller()->current()->selection.size() >= 2);
        QTest::keyClick(table, Qt::Key_A, Qt::ControlModifier);
        QCOMPARE(window.controller()->current()->selection.size(), 4);
        QVERIFY(window.findChild<QLabel *>("statusText")->text().contains("4 selected"));
        const auto selection = window.controller()->current()->selection;
        auto *view = window.findChild<QToolButton *>("view");
        view->menu()->actions()[1]->trigger();
        auto *list = window.findChild<QListView *>("iconList");
        QVERIFY(list->isVisible());
        QCOMPARE(list->viewMode(), QListView::ListMode);
        QCOMPARE(window.controller()->current()->selection, selection);
        list->clearSelection();
        const auto listIndex = window.proxy()->index(1, 0);
        list->setFocus();
        window.activateWindow();
        QCoreApplication::processEvents();
        list->selectionModel()->setCurrentIndex(listIndex, QItemSelectionModel::ClearAndSelect);
        QCOMPARE(window.controller()->current()->selection.size(), 1);
        QTest::keyClick(list, Qt::Key_C, Qt::ControlModifier);
        QVERIFY(QApplication::clipboard()->mimeData());
        QCOMPARE(QApplication::clipboard()->mimeData()->urls().size(), 1);
        list->selectAll();
        view->menu()->actions()[2]->trigger();
        QCOMPARE(list->viewMode(), QListView::IconMode);
        QCOMPARE(list->iconSize(), QSize(48, 48));
        view->menu()->actions()[0]->trigger();
        QVERIFY(table->isVisible());
        QCOMPARE(window.controller()->current()->selection, selection);
    }
    void copyCutPasteAndConflictChoice() {
        QTemporaryDir temp(owned());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("source"));
        QVERIFY(dir.mkdir("target"));
        write(temp.filePath("source/a.txt"), "source bytes");
        MainWindow window;
        window.show();
        window.openInitial(Location::folder(temp.filePath("source")));
        ready(window);
        auto *table = window.findChild<QTableView *>("fileList");
        select(window, "a.txt");
        QCOMPARE(window.controller()->current()->selection.size(), 1);
        QTest::keyClick(table, Qt::Key_C, Qt::ControlModifier);
        QVERIFY(QApplication::clipboard()->mimeData());
        const auto urls = QApplication::clipboard()->mimeData()->urls();
        QCOMPARE(urls.size(), 1);
        QCOMPARE(urls.front().toLocalFile(), temp.filePath("source/a.txt"));
        window.controller()->navigate(Location::folder(temp.filePath("target")));
        ready(window);
        table->setFocus();
        QTest::keyClick(table, Qt::Key_V, Qt::ControlModifier);
        QTRY_VERIFY(QFileInfo::exists(temp.filePath("target/a.txt")));
        QTRY_VERIFY(!window.operations()->busy());
        ready(window);
        write(temp.filePath("target/a.txt"), "old");
        QTimer timer;
        answerMessage(timer, "Replace files");
        table->setFocus();
        QTest::keyClick(table, Qt::Key_V, Qt::ControlModifier);
        QTRY_VERIFY(!timer.isActive());
        QTRY_VERIFY(!window.operations()->busy());
        ready(window);
        QFile target(temp.filePath("target/a.txt"));
        QVERIFY(target.open(QIODevice::ReadOnly));
        QCOMPARE(target.readAll(), QByteArray("source bytes"));
        target.close();
        write(temp.filePath("source/move.txt"), "move");
        window.controller()->navigate(Location::folder(temp.filePath("source")));
        ready(window);
        select(window, "move.txt");
        QCOMPARE(window.controller()->current()->selection, QStringList{temp.filePath("source/move.txt")});
        QTest::keyClick(table, Qt::Key_X, Qt::ControlModifier);
        window.controller()->navigate(Location::folder(temp.filePath("target")));
        ready(window);
        table->setFocus();
        QTest::keyClick(table, Qt::Key_V, Qt::ControlModifier);
        QTRY_VERIFY(QFileInfo::exists(temp.filePath("target/move.txt")));
        QTRY_VERIFY(!window.operations()->busy());
        QVERIFY(!QFileInfo::exists(temp.filePath("source/move.txt")));
    }
    void permanentDeleteCancelThenConfirm() {
        QTemporaryDir temp(owned());
        QVERIFY(temp.isValid());
        write(temp.filePath("safe.txt"), "owned disposable");
        MainWindow window;
        window.show();
        window.openInitial(Location::folder(temp.path()));
        ready(window);
        select(window, "safe.txt");
        auto *table = window.findChild<QTableView *>("fileList");
        QTimer cancel;
        answerMessage(cancel, "Cancel");
        QTest::keyClick(table, Qt::Key_Delete, Qt::ShiftModifier);
        QVERIFY(QFileInfo::exists(temp.filePath("safe.txt")));
        QVERIFY(!window.operations()->busy());
        QTimer yes;
        answerMessage(yes, "Yes");
        select(window, "safe.txt");
        QCOMPARE(window.controller()->current()->selection.size(), 1);
        QTest::keyClick(table, Qt::Key_Delete, Qt::ShiftModifier);
        QTRY_VERIFY(!QFileInfo::exists(temp.filePath("safe.txt")));
        QTRY_VERIFY(!window.operations()->busy());
        ready(window);
        QCOMPARE(window.fileModel()->rowCount(), 0);
    }
    void incomingLocalUrlDropAndOutgoingMime() {
        QTemporaryDir temp(owned());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("source"));
        QVERIFY(dir.mkdir("target"));
        write(temp.filePath(QString::fromUtf8("source/한글.txt")), "drop bytes");
        MainWindow window;
        window.show();
        window.openInitial(Location::folder(temp.filePath("target")));
        ready(window);
        auto *table = window.findChild<QTableView *>("fileList");
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(temp.filePath(QString::fromUtf8("source/한글.txt")))});
        QDragEnterEvent enter(QPoint(30, 60), Qt::CopyAction, &mime, Qt::LeftButton, Qt::ControlModifier);
        QApplication::sendEvent(table->viewport(), &enter);
        QVERIFY(enter.isAccepted());
        QDropEvent drop(QPointF(30, 60), Qt::CopyAction, &mime, Qt::LeftButton, Qt::ControlModifier);
        QApplication::sendEvent(table->viewport(), &drop);
        QVERIFY(drop.isAccepted());
        QTRY_VERIFY(QFileInfo::exists(temp.filePath(QString::fromUtf8("target/한글.txt"))));
        QTRY_VERIFY(!window.operations()->busy());
        ready(window);
        std::unique_ptr<QMimeData> outgoing(
            window.proxy()->mimeData({window.proxy()->index(0, 0), window.proxy()->index(0, 1)}));
        QCOMPARE(outgoing->urls().size(), 1);
        QCOMPARE(outgoing->urls().front().toLocalFile(), temp.filePath(QString::fromUtf8("target/한글.txt")));
        QMimeData remote;
        remote.setUrls({QUrl("https://example.invalid/a.txt")});
        QVERIFY(std::holds_alternative<Error>(localPaths(&remote)));
    }
    void internalMoveDropIntoFolder() {
        QTemporaryDir temp(owned());
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir("folder"));
        write(temp.filePath("source.txt"), "internal move");
        MainWindow window;
        window.show();
        window.openInitial(Location::folder(temp.path()));
        ready(window);
        auto *table = window.findChild<QTableView *>("fileList");
        const auto source = named(window, "source.txt"), folder = named(window, "folder");
        QVERIFY(source.isValid());
        QVERIFY(folder.isValid());
        std::unique_ptr<QMimeData> mime(window.proxy()->mimeData({source}));
        const auto pos = table->visualRect(folder).center();
        QDragEnterEvent enter(pos, Qt::MoveAction, mime.get(), Qt::LeftButton, Qt::ShiftModifier);
        QApplication::sendEvent(table->viewport(), &enter);
        QVERIFY(enter.isAccepted());
        QDropEvent drop(QPointF(pos), Qt::MoveAction, mime.get(), Qt::LeftButton, Qt::ShiftModifier);
        QApplication::sendEvent(table->viewport(), &drop);
        QVERIFY(drop.isAccepted());
        QTRY_VERIFY(QFileInfo::exists(temp.filePath("folder/source.txt")));
        QTRY_VERIFY(!window.operations()->busy());
        QVERIFY(!QFileInfo::exists(temp.filePath("source.txt")));
    }
};
QTEST_MAIN(ManagementUiTest)
#include "test_management_ui.moc"
