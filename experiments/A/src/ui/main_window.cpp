#include "explorer/main_window.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTabBar>
#include <QTableView>
#include <QToolButton>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace explorer {
namespace {
QToolButton *button(QWidget *parent, const QString &text, const QString &name, const QString &tooltip) {
    auto *result = new QToolButton(parent);
    result->setText(text);
    result->setObjectName(name);
    result->setAccessibleName(tooltip);
    result->setToolTip(tooltip);
    result->setCursor(Qt::PointingHandCursor);
    result->setMinimumSize(34, 32);
    return result;
}
Location itemLocation(const QTreeWidgetItem *item) {
    return item->data(0, Qt::UserRole + 1).toBool()
               ? Location{}
               : Location::folder(item->data(0, Qt::UserRole).toString());
}
QString locationKey(const Location &location) {
    return location.computer ? QStringLiteral(":computer:") : location.path;
}
} // namespace
MainWindow::MainWindow(std::shared_ptr<Platform> platform, QWidget *parent)
    : QMainWindow(parent), controller_(new BrowserController(platform, this)), model_(new FileModel(this)),
      proxy_(new FileProxy(this)), operations_(new OperationManager(platform, this)) {
    proxy_->setSourceModel(model_);
    createUi();
    connect(controller_, &BrowserController::tabsChanged, this, &MainWindow::rebuildTabs);
    connect(controller_, &BrowserController::currentChanged, this, [this] { synchronize(true); });
    connect(controller_, &BrowserController::tabChanged, this, [this](const QUuid &id) {
        if (id == controller_->currentId())
            synchronize();
    });
    connect(controller_, &BrowserController::treeReady, this, &MainWindow::receiveTree);
    connect(operations_, &OperationManager::finished, this, &MainWindow::operationFinished);
    connect(operations_, &OperationManager::progressChanged, this, &MainWindow::mutationProgress);
    connect(operations_, &OperationManager::busyChanged, this, [this](bool busy) {
        for (auto *control : mutationButtons_)
            control->setEnabled(!busy && controller_->capabilities().fileOperations);
        if (!busy) {
            progress_->hide();
            updateStatus();
        }
    });
    setWindowTitle(QStringLiteral("Explorer A"));
    resize(1240, 900);
}
void MainWindow::openInitial(const Location &location) {
    controller_->addTab(location);
}
void MainWindow::createUi() {
    setObjectName(QStringLiteral("explorerWindow"));
    auto *root = new QWidget(this);
    auto *layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto *tabRow = new QWidget(root);
    tabRow->setObjectName(QStringLiteral("tabRow"));
    auto *tabLayout = new QHBoxLayout(tabRow);
    tabLayout->setContentsMargins(10, 6, 10, 0);
    tabLayout->setSpacing(6);
    tabs_ = new QTabBar(tabRow);
    tabs_->setObjectName(QStringLiteral("tabs"));
    tabs_->setAccessibleName(QStringLiteral("Folder tabs"));
    tabs_->setTabsClosable(true);
    tabs_->setExpanding(false);
    tabs_->setDrawBase(false);
    tabs_->setElideMode(Qt::ElideRight);
    tabLayout->addWidget(tabs_);
    auto *plus =
        button(tabRow, QStringLiteral("+"), QStringLiteral("newTab"), QStringLiteral("New tab (Ctrl+T)"));
    tabLayout->addWidget(plus);
    tabLayout->addStretch();
    layout->addWidget(tabRow);
    connect(plus, &QToolButton::clicked, this, [this] { controller_->addTab(); });
    connect(tabs_, &QTabBar::currentChanged, this, [this](int index) {
        if (!synchronizing_ && index >= 0)
            controller_->setCurrent(tabs_->tabData(index).toUuid());
    });
    connect(tabs_, &QTabBar::tabCloseRequested, this,
            [this](int index) { controller_->closeTab(tabs_->tabData(index).toUuid()); });

    auto *navigation = new QWidget(root);
    navigation->setObjectName(QStringLiteral("navigationRow"));
    auto *navigationLayout = new QHBoxLayout(navigation);
    navigationLayout->setContentsMargins(14, 9, 14, 9);
    navigationLayout->setSpacing(9);
    back_ =
        button(navigation, QStringLiteral("←"), QStringLiteral("back"), QStringLiteral("Back (Alt+Left)"));
    forward_ = button(navigation, QStringLiteral("→"), QStringLiteral("forward"),
                      QStringLiteral("Forward (Alt+Right)"));
    up_ = button(navigation, QStringLiteral("↑"), QStringLiteral("up"), QStringLiteral("Up (Alt+Up)"));
    auto *refresh =
        button(navigation, QStringLiteral("⟳"), QStringLiteral("refresh"), QStringLiteral("Refresh (F5)"));
    for (auto *b : {back_, forward_, up_, refresh})
        navigationLayout->addWidget(b);
    address_ = new QLineEdit(navigation);
    address_->setObjectName(QStringLiteral("address"));
    address_->setAccessibleName(QStringLiteral("Address"));
    address_->setPlaceholderText(QStringLiteral("Enter a folder path"));
    address_->setMinimumHeight(34);
    address_->setClearButtonEnabled(true);
    navigationLayout->addWidget(address_, 3);
    search_ = new QLineEdit(navigation);
    search_->setObjectName(QStringLiteral("search"));
    search_->setAccessibleName(QStringLiteral("Search current folder"));
    search_->setPlaceholderText(QStringLiteral("Search current folder"));
    search_->setClearButtonEnabled(true);
    search_->setMinimumHeight(34);
    navigationLayout->addWidget(search_, 1);
    layout->addWidget(navigation);
    connect(back_, &QToolButton::clicked, controller_, &BrowserController::back);
    connect(forward_, &QToolButton::clicked, controller_, &BrowserController::forward);
    connect(up_, &QToolButton::clicked, controller_, &BrowserController::up);
    connect(refresh, &QToolButton::clicked, controller_, &BrowserController::refresh);
    connect(address_, &QLineEdit::returnPressed, this, [this] {
        const auto text = address_->text().trimmed();
        if (text.compare(QStringLiteral("This PC"), Qt::CaseInsensitive) == 0)
            controller_->navigate({});
        else if (!text.isEmpty())
            controller_->navigate(Location::folder(text));
    });
    connect(search_, &QLineEdit::textChanged, controller_, &BrowserController::setQuery);

    auto *commands = new QWidget(root);
    commands->setObjectName(QStringLiteral("commandRow"));
    auto *commandLayout = new QHBoxLayout(commands);
    commandLayout->setContentsMargins(15, 7, 15, 7);
    commandLayout->setSpacing(5);
    const auto mutation = [this, commands, commandLayout](const QString &text, const QString &name,
                                                          const QString &hint, auto callback) {
        auto *control = button(commands, text, name, hint);
        mutationButtons_.push_back(control);
        control->setEnabled(controller_->capabilities().fileOperations);
        commandLayout->addWidget(control);
        connect(control, &QToolButton::clicked, this, callback);
        return control;
    };
    mutation(QStringLiteral("⊕ New"), "newFolder", "New folder (Ctrl+Shift+N)", [this] { newFolder(); });
    mutation(QStringLiteral("Cut"), "cut", "Cut (Ctrl+X)", [this] { copySelection(true); });
    mutation(QStringLiteral("Copy"), "copy", "Copy (Ctrl+C)", [this] { copySelection(false); });
    mutation(QStringLiteral("Paste"), "paste", "Paste (Ctrl+V)", [this] { paste(); });
    mutation(QStringLiteral("Rename"), "rename", "Rename (F2)", [this] { renameSelected(); });
    mutation(QStringLiteral("Delete"), "delete", "Move to recycle bin (Delete)",
             [this] { deleteSelected(false); });
    auto *open = button(commands, QStringLiteral("Open"), QStringLiteral("open"),
                        QStringLiteral("Open selected item (Enter)"));
    connect(open, &QToolButton::clicked, this, [this] { activateIndex(currentView()->currentIndex()); });
    commandLayout->addWidget(open);
    auto *sort =
        button(commands, QStringLiteral("⇅  Sort ▾"), QStringLiteral("sort"), QStringLiteral("Sort"));
    auto *sortMenu = new QMenu(sort);
    const QList<QPair<QString, int>> columns{{QStringLiteral("Name"), FileModel::Name},
                                             {QStringLiteral("Size"), FileModel::Size},
                                             {QStringLiteral("Date modified"), FileModel::Modified}};
    for (const auto &[label, column] : columns) {
        connect(sortMenu->addAction(label), &QAction::triggered, this,
                [this, column] { table_->sortByColumn(column, Qt::AscendingOrder); });
    }
    sortMenu->addSeparator();
    connect(sortMenu->addAction(QStringLiteral("Ascending")), &QAction::triggered, this, [this] {
        table_->sortByColumn(table_->horizontalHeader()->sortIndicatorSection(), Qt::AscendingOrder);
    });
    connect(sortMenu->addAction(QStringLiteral("Descending")), &QAction::triggered, this, [this] {
        table_->sortByColumn(table_->horizontalHeader()->sortIndicatorSection(), Qt::DescendingOrder);
    });
    sort->setMenu(sortMenu);
    sort->setPopupMode(QToolButton::InstantPopup);
    commandLayout->addWidget(sort);
    viewButton_ =
        button(commands, QStringLiteral("☰  Details ▾"), QStringLiteral("view"), QStringLiteral("View mode"));
    auto *viewMenu = new QMenu(viewButton_);
    const QStringList viewLabels{"Details", "List", "Icons"};
    for (int mode = 0; mode < viewLabels.size(); ++mode)
        connect(viewMenu->addAction(viewLabels[mode]), &QAction::triggered, this,
                [this, mode] { controller_->setViewMode(mode); });
    viewButton_->setMenu(viewMenu);
    viewButton_->setPopupMode(QToolButton::InstantPopup);
    commandLayout->addWidget(viewButton_);
    auto *more = button(commands, QStringLiteral("•••"), "more", "More file actions");
    auto *moreMenu = new QMenu(more);
    connect(moreMenu->addAction("Copy to…"), &QAction::triggered, this, [this] { transferTo(false); });
    connect(moreMenu->addAction("Move to…"), &QAction::triggered, this, [this] { transferTo(true); });
    connect(moreMenu->addAction("Delete permanently…"), &QAction::triggered, this,
            [this] { deleteSelected(true); });
    connect(moreMenu->addAction("Retry retained cleanup…"), &QAction::triggered, this, [this] {
        const auto paths = operations_->retainedCleanup();
        if (paths.isEmpty()) {
            showError("No retained operation workspaces need cleanup in this session.");
            return;
        }
        if (QMessageBox::warning(
                this, "Clean retained workspaces",
                "Permanently remove the retained operation workspaces, including any backup copies?\n\n" +
                    paths.join('\n'),
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes)
            submit({OperationKind::Cleanup, paths, {}, {}, CollisionPolicy::Ask, true});
    });
    more->setMenu(moreMenu);
    more->setPopupMode(QToolButton::InstantPopup);
    commandLayout->addWidget(more);
    commandLayout->addStretch();
    auto *info = new QLabel(QStringLiteral("Local files"), commands);
    info->setObjectName(QStringLiteral("quietLabel"));
    commandLayout->addWidget(info);
    layout->addWidget(commands);
    error_ = new QLabel(root);
    error_->setObjectName(QStringLiteral("errorBanner"));
    error_->setAccessibleName(QStringLiteral("Folder error"));
    error_->setWordWrap(true);
    error_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    error_->hide();
    layout->addWidget(error_);

    auto *splitter = new QSplitter(root);
    splitter->setObjectName(QStringLiteral("contentSplitter"));
    tree_ = new QTreeWidget(splitter);
    tree_->setObjectName(QStringLiteral("navigationTree"));
    tree_->setAccessibleName(QStringLiteral("Navigation tree"));
    tree_->setHeaderHidden(true);
    tree_->setMinimumWidth(150);
    tree_->setIndentation(17);
    tree_->setIconSize(QSize(18, 18));
    tree_->setUniformRowHeights(true);
    addTreeLocation(nullptr, QStringLiteral("Home"), Location::folder(QDir::homePath()));
    addTreeLocation(nullptr, QStringLiteral("This PC"), {});
    connect(tree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item) {
        if (!item->data(0, Qt::UserRole + 2).toBool())
            controller_->navigate(itemLocation(item));
    });
    connect(tree_, &QTreeWidget::itemExpanded, this, &MainWindow::loadTreeItem);
    views_ = new QStackedWidget(splitter);
    table_ = new QTableView(views_);
    table_->setObjectName(QStringLiteral("fileList"));
    table_->setAccessibleName(QStringLiteral("Files"));
    table_->setModel(proxy_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setShowGrid(false);
    table_->setAlternatingRowColors(false);
    table_->setSortingEnabled(true);
    table_->sortByColumn(FileModel::Name, Qt::AscendingOrder);
    table_->setWordWrap(false);
    table_->verticalHeader()->hide();
    table_->verticalHeader()->setDefaultSectionSize(30);
    table_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(FileModel::Name, QHeaderView::Stretch);
    table_->setColumnWidth(FileModel::Modified, 185);
    table_->setColumnWidth(FileModel::Type, 130);
    table_->setColumnWidth(FileModel::Size, 105);
    views_->addWidget(table_);
    list_ = new QListView(views_);
    list_->setObjectName("iconList");
    list_->setAccessibleName("Files list or icons");
    list_->setModel(proxy_);
    list_->setModelColumn(FileModel::Name);
    list_->setSelectionModel(table_->selectionModel());
    list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list_->setResizeMode(QListView::Adjust);
    views_->addWidget(list_);
    connect(list_, &QListView::doubleClicked, this, &MainWindow::activateIndex);
    tableDrop_ = new FileViewInput(table_, proxy_, this);
    listDrop_ = new FileViewInput(list_, proxy_, this);
    for (auto *filter : {tableDrop_, listDrop_}) {
        connect(filter, &FileViewInput::dropped, this,
                [this](const QStringList &sources, const QString &destination, bool move) {
                    submit({move ? OperationKind::Move : OperationKind::Copy,
                            sources,
                            destination,
                            {},
                            CollisionPolicy::Ask,
                            false});
                });
        connect(filter, &FileViewInput::rejected, this, &MainWindow::showError);
        connect(filter, &FileViewInput::command, this, [this](FileCommand command) {
            switch (command) {
            case FileCommand::Open:
                activateIndex(currentView()->currentIndex());
                break;
            case FileCommand::Copy:
                copySelection(false);
                break;
            case FileCommand::Cut:
                copySelection(true);
                break;
            case FileCommand::Paste:
                paste();
                break;
            case FileCommand::SelectAll:
                currentView()->selectAll();
                break;
            case FileCommand::Rename:
                renameSelected();
                break;
            case FileCommand::Trash:
                deleteSelected(false);
                break;
            case FileCommand::Remove:
                deleteSelected(true);
                break;
            }
        });
    }
    connect(table_, &QTableView::doubleClicked, this, &MainWindow::activateIndex);
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
        if (synchronizing_)
            return;
        QStringList paths;
        for (const auto &index : table_->selectionModel()->selectedIndexes())
            if (index.column() == FileModel::Name)
                if (const auto *entry = model_->entry(proxy_->mapToSource(index).row()))
                    if (!paths.contains(entry->path))
                        paths.push_back(entry->path);
        controller_->setSelection(paths);
        updateStatus();
    });
    connect(table_->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this,
            [this](int column, Qt::SortOrder order) {
                if (!synchronizing_)
                    controller_->setSort(column, order);
            });
    splitter->setSizes({210, 1030});
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);
    status_ = new QLabel(this);
    status_->setObjectName(QStringLiteral("statusText"));
    status_->setAccessibleName(QStringLiteral("Folder status"));
    statusBar()->addWidget(status_, 1);
    statusBar()->setSizeGripEnabled(false);
    progress_ = new QProgressBar(this);
    progress_->setObjectName(QStringLiteral("loadingProgress"));
    progress_->setRange(0, 0);
    progress_->setFixedWidth(160);
    progress_->setMaximumHeight(13);
    progress_->hide();
    statusBar()->addPermanentWidget(progress_);
    setCentralWidget(root);

    const auto shortcut = [this](const QKeySequence &key, auto callback) {
        auto *s = new QShortcut(key, this);
        connect(s, &QShortcut::activated, this, callback);
    };
    shortcut(QKeySequence(QStringLiteral("Alt+Left")), [this] { controller_->back(); });
    shortcut(QKeySequence(QStringLiteral("Alt+Right")), [this] { controller_->forward(); });
    shortcut(QKeySequence(QStringLiteral("Alt+Up")), [this] { controller_->up(); });
    shortcut(QKeySequence(QStringLiteral("Ctrl+T")), [this] { controller_->addTab(); });
    shortcut(QKeySequence(QStringLiteral("Ctrl+W")),
             [this] { controller_->closeTab(controller_->currentId()); });
    shortcut(QKeySequence(QStringLiteral("Ctrl+Tab")), [this] {
        if (tabs_->count())
            tabs_->setCurrentIndex((tabs_->currentIndex() + 1) % tabs_->count());
    });
    shortcut(QKeySequence(QStringLiteral("Ctrl+L")), [this] {
        address_->setFocus();
        address_->selectAll();
    });
    shortcut(QKeySequence(QStringLiteral("Ctrl+F")), [this] {
        search_->setFocus();
        search_->selectAll();
    });
    shortcut(QKeySequence(Qt::Key_F5), [this] { controller_->refresh(); });
    shortcut(QKeySequence("Ctrl+Shift+N"), [this] { newFolder(); });
    shortcut(QKeySequence(Qt::Key_Escape), [this] {
        search_->clear();
        currentView()->setFocus();
    });

    setStyleSheet(QStringLiteral(R"(
QMainWindow, QWidget { background: #191919; color: #e9e9e9; font-family: "Segoe UI", "Noto Sans", sans-serif; font-size: 10pt; }
#tabRow { background: #10171c; } #navigationRow { background: #202327; }
QTabBar::tab { background: #151c22; border-radius: 7px; padding: 9px 18px; min-width: 155px; max-width: 240px; margin-right: 4px; }
QTabBar::tab:selected { background: #292c30; } QTabBar::tab:hover { background: #303439; }
QToolButton { border: 1px solid transparent; border-radius: 5px; padding: 3px 7px; background: transparent; }
QToolButton:hover { background: #33373b; } QToolButton:pressed { background: #41474b; } QToolButton:disabled { color: #727578; }
QLineEdit { background: #2a2d31; border: 1px solid #36393d; border-radius: 5px; padding: 1px 10px; selection-background-color: #305c80; }
QLineEdit:focus { border-bottom: 2px solid #70b7e4; }
#commandRow { border-bottom: 1px solid #383838; }
#quietLabel { color: #a2a2a2; } #errorBanner { background: #49352b; color: #ffe3cb; padding: 10px 18px; border-bottom: 1px solid #75533f; }
QTreeWidget { border: 0; border-right: 1px solid #303030; padding: 12px 5px; background: #1c1c1c; }
QTreeWidget::item { padding: 6px 3px; border-radius: 4px; } QTreeWidget::item:hover { background: #303030; } QTreeWidget::item:selected { background: #353b40; }
QTableView, QListView { background: #191919; border: 0; padding: 0 12px; selection-background-color: #334c60; selection-color: #ffffff; }
QTableView::item, QListView::item { border: 0; padding: 3px 4px; } QTableView::item:hover, QListView::item:hover { background: #2a2d30; }
QHeaderView::section { background: #191919; color: #bfbfbf; border: 0; border-right: 1px solid #333333; padding: 7px 8px; }
QStatusBar { background: #202020; border-top: 1px solid #2b2b2b; min-height: 26px; padding-left: 12px; } QStatusBar::item { border: 0; }
QMenu { background: #292929; border: 1px solid #484848; padding: 5px; } QMenu::item { padding: 7px 24px; } QMenu::item:selected { background: #414141; }
QSplitter::handle { background: #303030; width: 1px; }
QScrollBar:vertical { background: #202020; width: 11px; } QScrollBar::handle:vertical { background: #555555; border-radius: 4px; min-height: 25px; }
QProgressBar { background: #353535; border: 0; border-radius: 3px; } QProgressBar::chunk { background: #71bce8; }
)"));
}
void MainWindow::rebuildTabs() {
    synchronizing_ = true;
    const QSignalBlocker blocker(tabs_);
    while (tabs_->count())
        tabs_->removeTab(0);
    for (const auto &id : controller_->order()) {
        const auto *state = controller_->tab(id);
        const int index = tabs_->addTab(state->location.title());
        tabs_->setTabData(index, id);
        tabs_->setTabToolTip(index, state->location.display());
        if (id == controller_->currentId())
            tabs_->setCurrentIndex(index);
    }
    synchronizing_ = false;
}
void MainWindow::synchronize(bool switchTab) {
    const auto *state = controller_->current();
    if (!state)
        return;
    synchronizing_ = true;
    const bool newContent = renderedId_ != state->id || renderedRevision_ != state->revision;
    if (newContent) {
        model_->setEntries(state->listing.entries);
        renderedRevision_ = state->revision;
        renderedId_ = state->id;
    }
    proxy_->setQuery(state->query);
    if (switchTab || (newContent && !state->loading)) {
        const QSignalBlocker blocker(address_);
        address_->setText(state->location.display());
        address_->setCursorPosition(0);
    }
    {
        const QSignalBlocker blocker(search_);
        search_->setText(state->query);
    }
    table_->sortByColumn(state->sortColumn, state->sortOrder);
    setViewMode(state->viewMode);
    tableDrop_->setLocation(state->location);
    listDrop_->setLocation(state->location);
    if (switchTab || newContent) {
        table_->clearSelection();
        for (int row = 0; row < proxy_->rowCount(); ++row) {
            const auto index = proxy_->index(row, 0);
            const auto *item = model_->entry(proxy_->mapToSource(index).row());
            if (state->selection.contains(item->path))
                table_->selectionModel()->select(index,
                                                 QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }
    back_->setEnabled(state->history.canBack());
    forward_->setEnabled(state->history.canForward());
    up_->setEnabled(!state->location.computer);
    if (!operations_->busy()) {
        progress_->setRange(0, 0);
        progress_->setVisible(state->loading);
    }
    if (state->error)
        showError(state->error->message());
    else if (!operationError_.isEmpty())
        showError(operationError_);
    else if (!operations_->busy())
        error_->hide();
    setWindowTitle(state->location.title() + QStringLiteral(" — Explorer A"));
    synchronizing_ = false;
    updateStatus();
}
void MainWindow::updateStatus() {
    const auto *state = controller_->current();
    if (!state)
        return;
    qint64 total = 0, selected = 0;
    int selectedCount = 0;
    for (const auto &item : model_->entries()) {
        if (item.size >= 0)
            total += item.size;
        if (state->selection.contains(item.path)) {
            ++selectedCount;
            if (item.size >= 0)
                selected += item.size;
        }
    }
    QString text = state->query.isEmpty()
                       ? QStringLiteral("%1 items").arg(model_->rowCount())
                       : QStringLiteral("%1 of %2 items").arg(proxy_->rowCount()).arg(model_->rowCount());
    text += QStringLiteral("  ·  ") + formatBytes(total);
    if (selectedCount)
        text += QStringLiteral("  |  %1 selected  ·  %2").arg(selectedCount).arg(formatBytes(selected));
    if (state->loading)
        text += QStringLiteral("  |  Loading folder…");
    if (!operationStatus_.isEmpty())
        text += "  |  " + operationStatus_;
    if (!state->listing.warnings.isEmpty())
        text += QStringLiteral("  |  Some metadata unavailable");
    status_->setText(text);
}
void MainWindow::activateIndex(const QModelIndex &index) {
    if (index.isValid())
        if (const auto *item = model_->entry(proxy_->mapToSource(index).row()))
            controller_->activate(*item);
}
void MainWindow::addTreeLocation(QTreeWidgetItem *parent, const QString &title, const Location &location) {
    auto *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree_);
    item->setText(0, title);
    item->setIcon(0, style()->standardIcon(location.computer ? QStyle::SP_ComputerIcon : QStyle::SP_DirIcon));
    item->setData(0, Qt::UserRole, location.path);
    item->setData(0, Qt::UserRole + 1, location.computer);
    item->setData(0, Qt::UserRole + 3, false);
    item->setToolTip(0, location.display());
    auto *placeholder = new QTreeWidgetItem(item);
    placeholder->setText(0, QStringLiteral("Loading…"));
    placeholder->setData(0, Qt::UserRole + 2, true);
}
void MainWindow::loadTreeItem(QTreeWidgetItem *item) {
    if (item->data(0, Qt::UserRole + 3).toBool())
        return;
    item->setData(0, Qt::UserRole + 3, true);
    controller_->loadTree(itemLocation(item));
}
void MainWindow::receiveTree(const Listing &listing, const QString &error) {
    QList<QTreeWidgetItem *> matches;
    QTreeWidgetItemIterator iterator(tree_);
    while (*iterator) {
        auto *item = *iterator;
        ++iterator;
        if (item->data(0, Qt::UserRole + 2).toBool() ||
            locationKey(itemLocation(item)) != locationKey(listing.location))
            continue;
        matches.push_back(item);
    }
    for (auto *item : matches) {
        qDeleteAll(item->takeChildren());
        if (!error.isEmpty()) {
            item->setToolTip(0, error);
            item->setData(0, Qt::UserRole + 3, false);
            continue;
        }
        for (const auto &entry : listing.entries)
            if (entry.directory && !entry.link)
                addTreeLocation(item, entry.name, Location::folder(entry.path));
    }
}
QAbstractItemView *MainWindow::currentView() const {
    return views_->currentIndex() == 0 ? static_cast<QAbstractItemView *>(table_)
                                       : static_cast<QAbstractItemView *>(list_);
}
void MainWindow::setViewMode(int mode) {
    views_->setCurrentIndex(mode == 0 ? 0 : 1);
    if (mode == 1) {
        list_->setViewMode(QListView::ListMode);
        list_->setFlow(QListView::TopToBottom);
        list_->setWrapping(false);
        list_->setGridSize({});
        list_->setIconSize({20, 20});
        list_->setSpacing(2);
    } else if (mode == 2) {
        list_->setViewMode(QListView::IconMode);
        list_->setFlow(QListView::LeftToRight);
        list_->setWrapping(true);
        list_->setGridSize({170, 105});
        list_->setIconSize({48, 48});
        list_->setSpacing(5);
    }
    const QStringList labels{"☰  Details ▾", "☷  List ▾", "▦  Icons ▾"};
    viewButton_->setText(labels[qBound(0, mode, 2)]);
}
void MainWindow::copySelection(bool cut) {
    const auto *state = controller_->current();
    if (!state || state->selection.isEmpty() || operations_->busy())
        return;
    auto *mime = new QMimeData;
    QList<QUrl> urls;
    for (const auto &path : state->selection)
        urls.push_back(QUrl::fromLocalFile(path));
    mime->setUrls(urls);
    mime->setData("application/x-explorer-a-operation", cut ? "move" : "copy");
    mime->setData("x-kde-cutselection", cut ? "1" : "0");
    QByteArray effect(4, char(0));
    effect[0] = cut ? 2 : 1;
    mime->setData("Preferred DropEffect", effect);
    QApplication::clipboard()->setMimeData(mime);
    operationStatus_ = QStringLiteral("%1 items %2").arg(state->selection.size()).arg(cut ? "cut" : "copied");
    updateStatus();
}
void MainWindow::paste() {
    const auto *state = controller_->current();
    if (!state || state->location.computer)
        return;
    const auto *mime = QApplication::clipboard()->mimeData();
    const auto parsed = localPaths(mime);
    if (const auto *error = std::get_if<Error>(&parsed)) {
        showError(error->message());
        return;
    }
    const auto effect = mime->data("Preferred DropEffect");
    const bool move = mime->data("application/x-explorer-a-operation") == "move" ||
                      mime->data("x-kde-cutselection") == "1" || (!effect.isEmpty() && effect[0] == 2);
    submit({move ? OperationKind::Move : OperationKind::Copy,
            std::get<QStringList>(parsed),
            state->location.path,
            {},
            CollisionPolicy::Ask,
            false});
}
void MainWindow::newFolder() {
    const auto *state = controller_->current();
    if (!state || state->location.computer || operations_->busy())
        return;
    const auto destination = state->location.path;
    bool accepted = false;
    const auto name =
        QInputDialog::getText(this, "New folder", "Folder name:", QLineEdit::Normal, "New folder", &accepted);
    if (accepted)
        submit({OperationKind::NewFolder, {}, destination, name, CollisionPolicy::Ask, false});
}
void MainWindow::renameSelected() {
    const auto *state = controller_->current();
    if (!state || state->selection.size() != 1 || operations_->busy())
        return;
    const auto sources = state->selection;
    bool accepted = false;
    const auto name = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal,
                                            QFileInfo(sources.front()).fileName(), &accepted);
    if (accepted)
        submit({OperationKind::Rename, sources, {}, name, CollisionPolicy::Ask, false});
}
void MainWindow::deleteSelected(bool permanent) {
    const auto *state = controller_->current();
    if (!state || state->selection.isEmpty() || operations_->busy())
        return;
    const auto sources = state->selection;
    if (!permanent && !controller_->capabilities().recycle) {
        showError("Recycle is unavailable on this platform. Nothing was deleted.");
        return;
    }
    const auto title = permanent ? "Permanently delete" : "Move to recycle bin";
    const auto text =
        permanent ? QStringLiteral("Permanently delete %1 selected items? This cannot be undone by this app.")
                        .arg(sources.size())
                  : QStringLiteral("Move %1 selected items to the system recycle bin?").arg(sources.size());
    if (QMessageBox::warning(this, title, text, QMessageBox::Yes | QMessageBox::Cancel,
                             QMessageBox::Cancel) != QMessageBox::Yes)
        return;
    submit({permanent ? OperationKind::Remove : OperationKind::Trash,
            sources,
            {},
            {},
            CollisionPolicy::Ask,
            permanent});
}
void MainWindow::transferTo(bool move) {
    const auto *state = controller_->current();
    if (!state || state->selection.isEmpty() || operations_->busy())
        return;
    const auto sources = state->selection;
    const auto destination = QFileDialog::getExistingDirectory(
        this, move ? "Move to folder" : "Copy to folder", state->location.path);
    if (!destination.isEmpty())
        submit({move ? OperationKind::Move : OperationKind::Copy,
                sources,
                destination,
                {},
                CollisionPolicy::Ask,
                false});
}
void MainWindow::submit(OperationRequest request) {
    if (operations_->busy()) {
        showError("A file operation is already in progress. Wait for completion.");
        return;
    }
    if (!controller_->capabilities().fileOperations) {
        showError("File operations are unsupported on this platform.");
        return;
    }
    operationError_.clear();
    operationStatus_.clear();
    error_->hide();
    lastRequest_ = request;
    operations_->start(request);
}
void MainWindow::showError(const QString &message) {
    error_->setText(message);
    error_->show();
}
void MainWindow::mutationProgress(const OperationProgress &state) {
    operationStatus_ = QStringLiteral("%1 · %2/%3 items · %4 / %5")
                           .arg(state.phase)
                           .arg(state.items)
                           .arg(state.totalItems)
                           .arg(formatBytes(state.bytes), formatBytes(state.totalBytes));
    if (state.totalBytes > 0) {
        progress_->setRange(0, 1000);
        progress_->setValue(
            int(qMin<qint64>(1000, qint64(1000.0 * double(state.bytes) / double(state.totalBytes)))));
    } else
        progress_->setRange(0, 0);
    progress_->show();
    updateStatus();
}
void MainWindow::operationFinished(const OperationResult &result) {
    progress_->hide();
    if (const auto *failure = std::get_if<Error>(&result)) {
        if (failure->code == ErrorCode::AlreadyExists &&
            (lastRequest_.kind == OperationKind::Copy || lastRequest_.kind == OperationKind::Move) &&
            lastRequest_.collision == CollisionPolicy::Ask) {
            QMessageBox dialog(QMessageBox::Warning, "File conflict",
                               failure->message() +
                                   "\n\nApply one choice to all conflicts. Replace overwrites existing "
                                   "regular file contents. Directory merging is unsupported.",
                               QMessageBox::NoButton, this);
            auto *skip = dialog.addButton("Skip all", QMessageBox::ActionRole);
            auto *replace = dialog.addButton("Replace files", QMessageBox::DestructiveRole);
            auto *cancel = dialog.addButton(QMessageBox::Cancel);
            dialog.setDefaultButton(cancel);
            dialog.exec();
            if (dialog.clickedButton() == skip || dialog.clickedButton() == replace) {
                auto retry = lastRequest_;
                retry.collision =
                    dialog.clickedButton() == skip ? CollisionPolicy::Skip : CollisionPolicy::Replace;
                retry.confirmedDestructive = dialog.clickedButton() == replace;
                submit(retry);
                return;
            }
            operationStatus_ = "Operation cancelled before changes";
        } else {
            operationError_ = failure->message();
            showError(operationError_);
            operationStatus_ = "Operation failed";
        }
    } else {
        const auto &report = std::get<OperationReport>(result);
        int completed = 0, skipped = 0, failed = 0;
        QStringList details;
        for (const auto &item : report.items) {
            if (item.succeeded)
                ++completed;
            if (item.skipped)
                ++skipped;
            if (item.error) {
                ++failed;
                details.push_back(item.error->message());
            }
            if (!item.retainedPaths.isEmpty())
                details.push_back("Retained for recovery:\n" + item.retainedPaths.join('\n'));
        }
        operationStatus_ = QStringLiteral("%1 completed · %2 skipped · %3 failed%4")
                               .arg(completed)
                               .arg(skipped)
                               .arg(failed)
                               .arg(report.cancelled ? " · cancelled" : "");
        operationError_ = details.join("\n\n");
        if (!operationError_.isEmpty())
            showError(operationError_);
        if (lastRequest_.kind == OperationKind::Move && report.success() && !skipped) {
            const auto clipboardPaths = localPaths(QApplication::clipboard()->mimeData());
            if (const auto *paths = std::get_if<QStringList>(&clipboardPaths);
                paths && *paths == lastRequest_.sources)
                QApplication::clipboard()->clear();
        }
    }
    controller_->refreshAll();
    updateStatus();
}
} // namespace explorer
