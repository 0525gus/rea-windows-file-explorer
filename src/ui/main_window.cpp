#include "explorer/main_window.hpp"
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTabBar>
#include <QTableView>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QtConcurrentRun>
#include <limits>
namespace explorer {
namespace {
QIcon lineIcon(const QString &symbol, QColor color = QColor("#dddddd"), int dimension = 24) {
    static QHash<QString, QIcon> cache;
    const QString cacheKey = symbol + color.name(QColor::HexArgb) + QString::number(dimension);
    if (cache.contains(cacheKey))
        return cache.value(cacheKey);
    QPixmap image(dimension, dimension);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(dimension / 24.0, dimension / 24.0);
    p.setPen(QPen(color, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (symbol == "back" || symbol == "forward" || symbol == "up") {
        QPainterPath path;
        path.moveTo(14, 6);
        path.lineTo(8, 12);
        path.lineTo(14, 18);
        if (symbol == "forward") {
            p.translate(24, 0);
            p.scale(-1, 1);
        }
        if (symbol == "up") {
            p.translate(24, 0);
            p.rotate(90);
        }
        p.drawPath(path);
    } else if (symbol == "refresh") {
        p.drawArc(QRectF(5, 5, 14, 14), 40 * 16, 290 * 16);
        p.drawLine(19, 4, 19, 9);
        p.drawLine(14, 8, 19, 9);
    } else if (symbol == "plus") {
        p.drawLine(6, 12, 18, 12);
        p.drawLine(12, 6, 12, 18);
    } else if (symbol == "close") {
        p.drawLine(7, 7, 17, 17);
        p.drawLine(17, 7, 7, 17);
    } else if (symbol == "sort") {
        p.drawLine(7, 5, 7, 19);
        p.drawLine(4, 16, 7, 19);
        p.drawLine(10, 16, 7, 19);
        p.drawLine(13, 6, 20, 6);
        p.drawLine(13, 11, 18, 11);
        p.drawLine(13, 16, 16, 16);
    } else if (symbol == "view") {
        for (int y : {6, 12, 18}) {
            p.drawLine(7, y, 20, y);
            p.drawPoint(3, y);
        }
    } else if (symbol == "details") {
        p.drawRoundedRect(QRectF(4, 5, 16, 14), 2, 2);
        p.drawLine(14, 5, 14, 19);
    } else if (symbol == "folder") {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e5b748"));
        p.drawRoundedRect(QRectF(2, 6, 10, 5), 2, 2);
        p.setBrush(QColor("#f6c453"));
        p.drawRoundedRect(QRectF(2, 9, 20, 12), 2, 2);
    } else if (symbol == "file") {
        p.drawRoundedRect(QRectF(6, 3, 12, 18), 1, 1);
        p.drawLine(9, 9, 15, 9);
        p.drawLine(9, 13, 15, 13);
        p.drawLine(9, 17, 13, 17);
    } else if (symbol == "drive") {
        p.drawRoundedRect(QRectF(3, 8, 18, 11), 2, 2);
        p.drawLine(3, 14, 21, 14);
        p.drawPoint(17, 17);
    } else {
        p.setFont(QFont(QStringLiteral("Segoe UI"), 13));
        p.drawText(image.rect(), Qt::AlignCenter, symbol);
    }
    QIcon result(image);
    cache.insert(cacheKey, result);
    return result;
}
QString sizeText(std::optional<quint64> bytes) {
    if (!bytes)
        return QStringLiteral("Unknown");
    if (*bytes < 1024)
        return QString::number(*bytes) + QStringLiteral(" B");
    if (*bytes < 1024 * 1024)
        return QString::number(static_cast<double>(*bytes) / 1024.0, 'f', 1) + QStringLiteral(" KB");
    if (*bytes < 1024ULL * 1024 * 1024)
        return QString::number(static_cast<double>(*bytes) / (1024.0 * 1024), 'f', 1) + QStringLiteral(" MB");
    return QString::number(static_cast<double>(*bytes) / (1024.0 * 1024 * 1024), 'f', 1) +
           QStringLiteral(" GB");
}
} // namespace
QVariant EntryModel::data(const QModelIndex &index, int role) const {
    const auto *value = entry(index.row());
    if (!value || !index.isValid())
        return {};
    if (role == Qt::UserRole)
        return value->key;
    if (role == Qt::UserRole + 1)
        return value->path;
    if (role == Qt::DecorationRole && index.column() == 0)
        return lineIcon(value->directory ? "folder" : "file");
    if (role == Qt::TextAlignmentRole && index.column() == 3)
        return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
    if (role == Qt::ToolTipRole)
        return value->path + (value->link ? QStringLiteral("\nSymbolic link / reparse point") : QString());
    if (role != Qt::DisplayRole)
        return {};
    switch (index.column()) {
    case 0:
        return value->name;
    case 1:
        return value->modifiedMs ? QDateTime::fromMSecsSinceEpoch(*value->modifiedMs)
                                       .toString(QStringLiteral("yyyy-MM-dd  HH:mm"))
                                 : QStringLiteral("Unknown");
    case 2:
        return value->typeName;
    case 3:
        return value->directory ? QString() : sizeText(value->sizeBytes);
    }
    return {};
}
QVariant EntryModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    static const QStringList labels{QStringLiteral("Name"), QStringLiteral("Date modified"),
                                    QStringLiteral("Type"), QStringLiteral("Size")};
    return section >= 0 && section < labels.size() ? labels[section] : QVariant();
}
void EntryModel::setEntries(QVector<Entry> entries) {
    beginResetModel();
    entries_ = std::move(entries);
    endResetModel();
}
const Entry *EntryModel::entry(int row) const {
    return row >= 0 && row < entries_.size() ? &entries_[row] : nullptr;
}
QToolButton *MainWindow::makeButton(const QString &symbol, const QString &label, QWidget *parent) {
    auto *button = new QToolButton(parent);
    button->setIcon(lineIcon(symbol));
    button->setIconSize(QSize(20, 20));
    button->setToolTip(label);
    button->setAccessibleName(label);
    button->setFixedSize(36, 34);
    button->setAutoRaise(true);
    return button;
}
MainWindow::MainWindow(std::shared_ptr<IPlatform> platform, QWidget *parent)
    : QMainWindow(parent), platform_(std::move(platform)), session_(new ExplorerSession(platform_, this)) {
    setObjectName("explorerWindow");
    setWindowTitle(QStringLiteral("REA Explorer"));
    resize(1240, 860);
    setMinimumSize(960, 600);
    treePool_.setMaxThreadCount(2);
    auto *root = new QWidget(this);
    auto *layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setCentralWidget(root);
    auto *tabRow = new QWidget(root);
    tabRow->setObjectName("tabRow");
    auto *tabLayout = new QHBoxLayout(tabRow);
    tabLayout->setContentsMargins(12, 4, 12, 0);
    tabLayout->setSpacing(3);
    tabs_ = new QTabBar(tabRow);
    tabs_->setObjectName("tabs");
    tabs_->setTabsClosable(true);
    tabs_->setExpanding(false);
    tabs_->setMovable(false);
    tabs_->setElideMode(Qt::ElideRight);
    auto *add = makeButton("plus", "New tab", tabRow);
    add->setObjectName("newTab");
    tabLayout->addWidget(tabs_);
    tabLayout->addWidget(add);
    tabLayout->addStretch();
    layout->addWidget(tabRow);
    auto *navigation = new QWidget(root);
    navigation->setObjectName("navigation");
    auto *nav = new QHBoxLayout(navigation);
    nav->setContentsMargins(12, 8, 14, 8);
    nav->setSpacing(5);
    back_ = makeButton("back", "Back (Alt+Left)", navigation);
    back_->setObjectName("back");
    forward_ = makeButton("forward", "Forward (Alt+Right)", navigation);
    forward_->setObjectName("forward");
    up_ = makeButton("up", "Up (Alt+Up)", navigation);
    up_->setObjectName("up");
    auto *refresh = makeButton("refresh", "Refresh (F5)", navigation);
    refresh->setObjectName("refresh");
    for (auto *button : {back_, forward_, up_, refresh})
        nav->addWidget(button);
    address_ = new QLineEdit(navigation);
    address_->setObjectName("address");
    address_->setAccessibleName("Address");
    address_->setClearButtonEnabled(false);
    address_->setMinimumWidth(250);
    address_->addAction(lineIcon("drive"), QLineEdit::LeadingPosition);
    nav->addWidget(address_, 1);
    search_ = new QLineEdit(navigation);
    search_->setObjectName("search");
    search_->setAccessibleName("Search this folder");
    search_->setPlaceholderText("Search this folder");
    search_->setClearButtonEnabled(true);
    search_->setFixedWidth(265);
    nav->addWidget(search_);
    layout->addWidget(navigation);
    auto *commands = new QWidget(root);
    commands->setObjectName("commands");
    auto *commandLayout = new QHBoxLayout(commands);
    commandLayout->setContentsMargins(14, 4, 14, 6);
    commandLayout->setSpacing(6);
    auto *fresh = makeButton("plus", "New folder", commands);
    fresh->setText("New");
    fresh->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    fresh->setFixedWidth(92);
    fresh->setEnabled(false);
    commandLayout->addWidget(fresh);
    for (const auto &pair : QVector<QPair<QString, QString>>{
             {"✂", "Cut"}, {"□", "Copy"}, {"▣", "Paste"}, {"⌑", "Rename"}, {"♲", "Recycle"}}) {
        auto *button = makeButton(pair.first, pair.second, commands);
        button->setEnabled(false);
        commandLayout->addWidget(button);
    }
    auto *sort = makeButton("sort", "Sort", commands);
    sort->setText("Sort");
    sort->setFixedWidth(90);
    sort->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    commandLayout->addWidget(sort);
    auto *view = makeButton("view", "View", commands);
    view->setText("View");
    view->setFixedWidth(90);
    view->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    commandLayout->addWidget(view);
    commandLayout->addStretch();
    auto *details = makeButton("details", "Details pane", commands);
    details->setText("Details");
    details->setFixedWidth(95);
    details->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    commandLayout->addWidget(details);
    layout->addWidget(commands);
    error_ = new QLabel(root);
    error_->setObjectName("errorBanner");
    error_->setWordWrap(true);
    error_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    error_->setVisible(false);
    layout->addWidget(error_);
    splitter_ = new QSplitter(Qt::Horizontal, root);
    splitter_->setChildrenCollapsible(false);
    tree_ = new QTreeWidget(splitter_);
    tree_->setObjectName("navigationTree");
    tree_->setAccessibleName("Folders");
    tree_->setHeaderHidden(true);
    tree_->setMinimumWidth(155);
    tree_->setIndentation(18);
    tree_->setRootIsDecorated(true);
    auto *computer = new QTreeWidgetItem(tree_, QStringList{"This PC"});
    computer->setIcon(0, lineIcon("drive"));
    computer->setData(0, Qt::UserRole, QStringLiteral("drives"));
    computer->setData(0, Qt::UserRole + 1, QStringLiteral("drives"));
    treeItems_.insert("drives", computer);
    new QTreeWidgetItem(computer, QStringList{"Loading…"});
    table_ = new QTableView(splitter_);
    table_->setObjectName("fileList");
    table_->setAccessibleName("Files");
    model_ = new EntryModel(this);
    table_->setModel(model_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setShowGrid(false);
    table_->setAlternatingRowColors(false);
    table_->verticalHeader()->hide();
    table_->verticalHeader()->setDefaultSectionSize(29);
    table_->setIconSize(QSize(21, 21));
    table_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table_->horizontalHeader()->setHighlightSections(false);
    table_->horizontalHeader()->setSectionsClickable(true);
    table_->horizontalHeader()->setSortIndicatorShown(true);
    table_->setColumnWidth(0, 310);
    table_->setColumnWidth(1, 175);
    table_->setColumnWidth(2, 145);
    table_->setColumnWidth(3, 95);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setMinimumWidth(410);
    details_ = new QWidget(splitter_);
    details_->setObjectName("detailsPane");
    details_->setMinimumWidth(210);
    auto *detailLayout = new QVBoxLayout(details_);
    detailLayout->setContentsMargins(22, 32, 18, 20);
    auto *folderImage = new QLabel(details_);
    folderImage->setPixmap(lineIcon("folder", QColor("#dddddd"), 94).pixmap(94, 94));
    folderImage->setAlignment(Qt::AlignCenter);
    detailLayout->addWidget(folderImage);
    detailLayout->addSpacing(22);
    detailName_ = new QLabel("This PC", details_);
    detailName_->setObjectName("detailName");
    detailName_->setWordWrap(true);
    detailName_->setStyleSheet("font-size:18px;font-weight:600;");
    detailLayout->addWidget(detailName_);
    detailMeta_ = new QLabel("Select a file to see its details.", details_);
    detailMeta_->setWordWrap(true);
    detailMeta_->setStyleSheet("color:#b5b5b5;");
    detailLayout->addWidget(detailMeta_);
    detailLayout->addStretch();
    splitter_->addWidget(tree_);
    splitter_->addWidget(table_);
    splitter_->addWidget(details_);
    splitter_->setStretchFactor(1, 1);
    splitter_->setSizes({190, 790, 250});
    layout->addWidget(splitter_, 1);
    status_ = new QLabel("Loading…", this);
    status_->setObjectName("status");
    statusBar()->addWidget(status_, 1);
    statusBar()->setSizeGripEnabled(true);
    statusBar()->setContentsMargins(8, 0, 8, 0);
    progress_ = new QProgressBar(this);
    progress_->setObjectName("loadProgress");
    progress_->setMaximumWidth(150);
    progress_->setMaximumHeight(12);
    progress_->setRange(0, 0);
    progress_->hide();
    statusBar()->addPermanentWidget(progress_);
    setStyleSheet(R"(
QMainWindow,QWidget{background:#191919;color:#ededed;font-family:"Segoe UI";font-size:12px;}
QLabel{background:transparent;}
QWidget#tabRow{background:#11181e;} QTabBar::tab{background:#171d23;color:#bdbdbd;min-width:145px;max-width:230px;padding:10px 14px;border-top-left-radius:8px;border-top-right-radius:8px;margin-right:3px;} QTabBar::tab:selected{background:#242424;color:white;}
QWidget#navigation{background:#202020;} QLineEdit{background:#2b2c30;border:1px solid transparent;border-radius:5px;padding:8px 10px;selection-background-color:#376899;} QLineEdit:focus{border-bottom:2px solid #75baff;}
QWidget#commands{border-bottom:1px solid #343434;} QToolButton{border:0;border-radius:4px;padding:4px;color:#e0e0e0;} QToolButton:hover{background:#353535;}QToolButton:pressed{background:#414141;}QToolButton:disabled{color:#696969;}
QTreeWidget{border:0;border-right:1px solid #333;padding-top:8px;background:#202020;outline:0;}QTreeWidget::item{padding:6px 4px;border-radius:4px;}QTreeWidget::item:selected{background:#3a3a3a;}QTreeWidget::item:hover{background:#303030;}
QTableView{border:0;background:#191919;selection-background-color:#344454;selection-color:white;outline:0;}QTableView::item{padding-left:7px;border:0;}QTableView::item:selected{background:#354756;} QHeaderView::section{background:#191919;color:#c8c8c8;border:0;border-right:1px solid #343434;border-bottom:1px solid #333;padding:7px 9px;text-align:left;}QHeaderView{background:#191919;}
QWidget#detailsPane{background:#202020;border-left:1px solid #333;}QStatusBar{background:#202020;border-top:1px solid #303030;}QStatusBar::item{border:0;}QLabel#errorBanner{background:#493b26;color:#ffe0ab;padding:10px 16px;border-bottom:1px solid #6a5437;}
QSplitter::handle{background:#303030;width:1px;}QScrollBar:vertical{background:#1c1c1c;width:10px;}QScrollBar::handle:vertical{background:#555;border-radius:4px;min-height:25px;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
QMenu{background:#292929;border:1px solid #454545;padding:6px;}QMenu::item{padding:7px 26px;}QMenu::item:selected{background:#414141;border-radius:3px;}
)");
    connect(session_, &ExplorerSession::tabsChanged, this, &MainWindow::syncTabs);
    connect(session_, &ExplorerSession::tabChanged, this, &MainWindow::render);
    connect(session_, &ExplorerSession::shutdownFinished, this, &MainWindow::finishClose);
    connect(add, &QToolButton::clicked, this, [this] { session_->createTab(); });
    connect(tabs_, &QTabBar::currentChanged, this, [this](int) {
        if (!updating_)
            render(currentTabId());
    });
    connect(tabs_, &QTabBar::tabCloseRequested, this, [this](int index) {
        if (tabs_->count() == 1) {
            close();
            return;
        }
        session_->closeTab(tabs_->tabData(index).toULongLong());
    });
    connect(back_, &QToolButton::clicked, this, [this] { session_->back(currentTabId()); });
    connect(forward_, &QToolButton::clicked, this, [this] { session_->forward(currentTabId()); });
    connect(up_, &QToolButton::clicked, this, [this] { session_->up(currentTabId()); });
    connect(refresh, &QToolButton::clicked, this, [this] { session_->refresh(currentTabId()); });
    connect(address_, &QLineEdit::returnPressed, this, &MainWindow::navigateAddress);
    connect(search_, &QLineEdit::textChanged, this, [this](const QString &value) {
        if (!updating_)
            session_->setSearch(currentTabId(), value);
    });
    connect(table_, &QTableView::doubleClicked, this, &MainWindow::activateRow);
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
        if (!updating_)
            updateSelection();
    });
    connect(table_->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int column) {
        if (column == 2)
            return;
        auto *state = session_->tab(currentTabId());
        if (!state)
            return;
        SortSpec sort = state->sort;
        const auto selected =
            column == 1 ? SortColumn::Modified : (column == 3 ? SortColumn::Size : SortColumn::Name);
        sort.ascending = sort.column == selected ? !sort.ascending : true;
        sort.column = selected;
        session_->setSort(state->id, sort);
    });
    connect(tree_, &QTreeWidget::itemExpanded, this, &MainWindow::requestTree);
    connect(tree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
        const auto path = item->data(0, Qt::UserRole + 1).toString();
        if (path == "drives")
            session_->navigate(currentTabId(), Location{});
        else if (!path.isEmpty())
            session_->navigate(currentTabId(), path);
    });
    connect(sort, &QToolButton::clicked, this, &MainWindow::showSortMenu);
    connect(view, &QToolButton::clicked, this, &MainWindow::showViewsMenu);
    connect(details, &QToolButton::clicked, this, [this] { details_->setVisible(!details_->isVisible()); });
    const auto shortcut = [this](const QKeySequence &keys, auto action) {
        auto *value = new QShortcut(keys, this);
        connect(value, &QShortcut::activated, this, action);
    };
    shortcut(QKeySequence("Alt+Left"), [this] { session_->back(currentTabId()); });
    shortcut(QKeySequence("Alt+Right"), [this] { session_->forward(currentTabId()); });
    shortcut(QKeySequence("Alt+Up"), [this] { session_->up(currentTabId()); });
    shortcut(QKeySequence("Alt+D"), [this] {
        address_->setFocus();
        address_->selectAll();
    });
    shortcut(QKeySequence("Ctrl+L"), [this] {
        address_->setFocus();
        address_->selectAll();
    });
    shortcut(QKeySequence("Ctrl+F"), [this] {
        search_->setFocus();
        search_->selectAll();
    });
    shortcut(QKeySequence("Ctrl+T"), [this] { session_->createTab(); });
    shortcut(QKeySequence("Ctrl+W"), [this] {
        if (tabs_->count() == 1)
            close();
        else
            session_->closeTab(currentTabId());
    });
    shortcut(QKeySequence("Ctrl+Tab"),
             [this] { tabs_->setCurrentIndex((tabs_->currentIndex() + 1) % tabs_->count()); });
    shortcut(QKeySequence("F5"), [this] { session_->refresh(currentTabId()); });
    auto *openShortcut = new QShortcut(QKeySequence(Qt::Key_Return), table_);
    openShortcut->setContext(Qt::WidgetShortcut);
    connect(openShortcut, &QShortcut::activated, this, [this] { activateRow(table_->currentIndex()); });
    progressTimer_.setInterval(120);
    connect(&progressTimer_, &QTimer::timeout, this, [this] {
        const auto *state = session_->tab(currentTabId());
        if (state && state->loading && state->examined)
            status_->setText(QStringLiteral("Loading… %1 items examined").arg(state->examined->load()));
    });
    progressTimer_.start();
    session_->createTab();
    computer->setExpanded(true);
}
MainWindow::~MainWindow() {
    for (const auto &stop : treeStops_)
        stop->request_stop();
    treePool_.waitForDone();
}
quint64 MainWindow::currentTabId() const {
    return tabs_->currentIndex() < 0 ? 0 : tabs_->tabData(tabs_->currentIndex()).toULongLong();
}
void MainWindow::openPath(const QString &path) {
    session_->navigate(currentTabId(), path);
}
void MainWindow::syncTabs() {
    const auto old = currentTabId();
    const int previousCount = tabs_->count(), previousIndex = tabs_->currentIndex();
    updating_ = true;
    QSignalBlocker blocker(tabs_);
    while (tabs_->count())
        tabs_->removeTab(0);
    int select = std::clamp(previousIndex, 0, std::max(0, static_cast<int>(session_->tabs().size()) - 1));
    for (const auto &state : session_->tabs()) {
        const int index = tabs_->addTab(lineIcon("folder"), state.location.label);
        tabs_->setTabData(index, QVariant::fromValue(state.id));
        auto *closeButton = makeButton("close", "Close tab", tabs_);
        closeButton->setFixedSize(22, 22);
        closeButton->setIconSize(QSize(16, 16));
        tabs_->setTabButton(index, QTabBar::RightSide, closeButton);
        connect(closeButton, &QToolButton::clicked, this, [this, id = state.id] {
            if (tabs_->count() == 1)
                close();
            else
                session_->closeTab(id);
        });
        if (state.id == old)
            select = index;
    }
    if (!session_->tabs().isEmpty()) {
        if (tabs_->count() > previousCount)
            select = tabs_->count() - 1;
        tabs_->setCurrentIndex(select);
    }
    updating_ = false;
    render(currentTabId());
}
void MainWindow::render(quint64 id) {
    const auto *state = session_->tab(id);
    if (!state)
        return;
    for (int i = 0; i < tabs_->count(); ++i)
        if (tabs_->tabData(i).toULongLong() == id)
            tabs_->setTabText(i, state->loading ? QStringLiteral("Loading…") : state->location.label);
    if (id != currentTabId())
        return;
    updating_ = true;
    QSignalBlocker searchBlock(search_);
    QSignalBlocker selectionBlock(table_->selectionModel());
    if (!address_->hasFocus() || !state->loading)
        address_->setText(state->error && !state->pendingInput.isEmpty()
                              ? state->pendingInput
                              : (state->location.kind == Location::Kind::Drives ? QStringLiteral("This PC")
                                                                                : state->location.path));
    search_->setText(state->query);
    search_->setPlaceholderText("Search " + state->location.label);
    model_->setEntries(state->visible);
    for (int row = 0; row < model_->rowCount(); ++row)
        if (state->selection.contains(model_->entry(row)->key))
            table_->selectionModel()->select(model_->index(row, 0),
                                             QItemSelectionModel::Select | QItemSelectionModel::Rows);
    const int column =
        state->sort.column == SortColumn::Name ? 0 : (state->sort.column == SortColumn::Modified ? 1 : 3);
    table_->horizontalHeader()->setSortIndicator(column, state->sort.ascending ? Qt::AscendingOrder
                                                                               : Qt::DescendingOrder);
    back_->setEnabled(state->history.canBack() && !state->loading);
    forward_->setEnabled(state->history.canForward() && !state->loading);
    up_->setEnabled(state->location.kind != Location::Kind::Drives && !state->loading);
    error_->setVisible(state->error.has_value());
    if (state->error)
        error_->setText(errorCodeName(state->error->code) + QStringLiteral(" — ") + state->error->message +
                        QStringLiteral("  Previous folder remains available. Correct the address or retry."));
    progress_->setVisible(state->loading);
    status_->setText(state->loading ? QStringLiteral("Loading…")
                                    : QStringLiteral("%1 items%2")
                                          .arg(state->visible.size())
                                          .arg(state->query.isEmpty()
                                                   ? QString()
                                                   : QStringLiteral(" matching “%1”").arg(state->query)));
    detailName_->setText(state->location.label);
    detailMeta_->setText(
        QStringLiteral("%1 items\n\nSelect a file to see its details.").arg(state->visible.size()));
    setWindowTitle(state->location.label + QStringLiteral(" — REA Explorer"));
    updating_ = false;
    updateSelection();
}
void MainWindow::navigateAddress() {
    session_->navigate(currentTabId(), address_->text());
    table_->setFocus();
}
void MainWindow::activateRow(const QModelIndex &index) {
    if (const auto *entry = model_->entry(index.row()))
        session_->openEntry(currentTabId(), entry->key);
}
void MainWindow::updateSelection() {
    QSet<QString> keys;
    quint64 bytes = 0;
    bool known = true;
    const Entry *single = nullptr;
    for (const auto &index : table_->selectionModel()->selectedRows()) {
        const auto *entry = model_->entry(index.row());
        if (!entry)
            continue;
        keys.insert(entry->key);
        single = entry;
        if (entry->sizeBytes) {
            if (*entry->sizeBytes > std::numeric_limits<quint64>::max() - bytes)
                known = false;
            else
                bytes += *entry->sizeBytes;
        } else if (!entry->directory)
            known = false;
    }
    session_->setSelection(currentTabId(), keys);
    const auto *state = session_->tab(currentTabId());
    if (!state || state->loading)
        return;
    if (!keys.isEmpty()) {
        status_->setText(QStringLiteral("%1 items   |   %2 selected%3")
                             .arg(model_->rowCount())
                             .arg(keys.size())
                             .arg(known ? QStringLiteral("   %1").arg(sizeText(bytes)) : QString()));
        detailName_->setText(keys.size() == 1 ? single->name
                                              : QStringLiteral("%1 items selected").arg(keys.size()));
        detailMeta_->setText(keys.size() == 1
                                 ? single->typeName + QStringLiteral("\n\n") + single->path +
                                       QStringLiteral("\n\n") + sizeText(single->sizeBytes)
                                 : QStringLiteral("Combined size: ") +
                                       sizeText(known ? std::optional<quint64>(bytes) : std::nullopt));
    }
}
void MainWindow::requestTree(QTreeWidgetItem *item) {
    const QString key = item->data(0, Qt::UserRole).toString(),
                  path = item->data(0, Qt::UserRole + 1).toString();
    if (key.isEmpty() || closing_ || treeStops_.contains(key) || item->data(0, Qt::UserRole + 2).toBool())
        return;
    if (treeActive_ >= 8) {
        item->setToolTip(0, "Tree request limit reached; collapse and retry.");
        return;
    }
    auto stop = std::make_shared<std::stop_source>();
    treeStops_.insert(key, stop);
    ++treeActive_;
    const auto adapter = platform_;
    auto *watcher = new QFutureWatcher<Result<Listing>>(this);
    connect(watcher, &QFutureWatcher<Result<Listing>>::finished, this, [this, watcher, key, stop] {
        const auto result = watcher->result();
        watcher->deleteLater();
        --treeActive_;
        treeStops_.remove(key);
        if (!closing_ && !stop->stop_requested()) {
            auto *target = treeItems_.value(key, nullptr);
            if (target) {
                qDeleteAll(target->takeChildren());
                if (const auto *error = failure(result)) {
                    target->setToolTip(0, error->message);
                    new QTreeWidgetItem(target, QStringList{errorCodeName(error->code)});
                } else {
                    target->setData(0, Qt::UserRole + 2, true);
                    for (const auto &entry : std::get<Listing>(result).entries)
                        if (entry.directory) {
                            auto *child = new QTreeWidgetItem(target, QStringList{entry.name});
                            child->setIcon(0, lineIcon("folder"));
                            child->setData(0, Qt::UserRole, entry.key);
                            child->setData(0, Qt::UserRole + 1, entry.path);
                            child->setToolTip(0, entry.path);
                            treeItems_.insert(entry.key, child);
                            if (!entry.link)
                                new QTreeWidgetItem(child, QStringList{"…"});
                        }
                }
            }
        }
        finishClose();
    });
    watcher->setFuture(QtConcurrent::run(&treePool_, [adapter, path, stop]() -> Result<Listing> {
        auto location = path == "drives" ? Result<Location>(Location{}) : adapter->parseLocation(path);
        if (const auto *error = failure(location))
            return *error;
        return adapter->list(std::get<Location>(location), stop->get_token());
    }));
}
void MainWindow::closeEvent(QCloseEvent *event) {
    if (finalClose_) {
        event->accept();
        return;
    }
    event->ignore();
    if (closing_)
        return;
    closing_ = true;
    setEnabled(false);
    for (const auto &stop : treeStops_)
        stop->request_stop();
    session_->shutdown();
    finishClose();
}
void MainWindow::finishClose() {
    if (closing_ && session_->idle() && treeActive_ == 0) {
        finalClose_ = true;
        QTimer::singleShot(0, this, &QWidget::close);
    }
}
void MainWindow::showSortMenu() {
    QMenu menu(this);
    for (const auto &pair : QVector<QPair<QString, SortColumn>>{{"Name", SortColumn::Name},
                                                                {"Size", SortColumn::Size},
                                                                {"Date modified", SortColumn::Modified}}) {
        auto *action = menu.addAction(pair.first);
        connect(action, &QAction::triggered, this,
                [this, column = pair.second] { session_->setSort(currentTabId(), {column, true}); });
    }
    menu.addSeparator();
    auto *descending = menu.addAction("Descending");
    connect(descending, &QAction::triggered, this, [this] {
        if (const auto *state = session_->tab(currentTabId())) {
            auto sort = state->sort;
            sort.ascending = false;
            session_->setSort(currentTabId(), sort);
        }
    });
    menu.exec(QCursor::pos());
}
void MainWindow::showViewsMenu() {
    QMenu menu(this);
    auto *details = menu.addAction("Details");
    details->setCheckable(true);
    details->setChecked(true);
    auto *icons = menu.addAction("Icons");
    icons->setEnabled(false);
    auto *list = menu.addAction("List");
    list->setEnabled(false);
    menu.exec(QCursor::pos());
}
QJsonObject MainWindow::snapshot() const {
    QJsonObject result;
    const auto *state = session_->tab(currentTabId());
    if (!state)
        return result;
    result["path"] = state->location.path;
    result["kind"] = state->location.kind == Location::Kind::Drives ? "drives" : "directory";
    result["loading"] = state->loading;
    result["count"] = state->visible.size();
    result["history_cursor"] = state->history.cursor();
    result["tab_count"] = session_->tabs().size();
    if (state->error)
        result["error"] =
            QJsonObject{{"code", errorCodeName(state->error->code)}, {"message", state->error->message}};
    QJsonArray entries;
    for (const auto &entry : state->visible) {
        QJsonObject row{{"name", entry.name},
                        {"path", entry.path},
                        {"directory", entry.directory},
                        {"link", entry.link},
                        {"identity", entry.identity}};
        row["size"] = entry.sizeBytes ? QJsonValue(static_cast<qint64>(*entry.sizeBytes)) : QJsonValue();
        row["modified_ms"] = entry.modifiedMs ? QJsonValue(*entry.modifiedMs) : QJsonValue();
        entries.append(row);
    }
    result["entries"] = entries;
    return result;
}
} // namespace explorer
