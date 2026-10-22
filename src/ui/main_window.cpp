#include "explorer/main_window.hpp"
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFile>
#include <QFileSystemWatcher>
#include <QImageReader>
#include <QPlainTextEdit>
#include <QSettings>
#include <QFocusEvent>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPersistentModelIndex>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QStyleOptionFrame>
#include <QTabBar>
#include <QTableView>
#include <QToolButton>
#include <QTreeWidget>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>
#include <QtConcurrentRun>
#include <limits>
#include <utility>
namespace explorer {
namespace {
template <class Base> class TransferView final : public Base {
  public:
    using Base::Base;
    std::function<bool(const QMimeData *, Qt::DropAction, int, bool)> receive;
    void startDrag(Qt::DropActions) override {
        QList<QUrl> urls;
        QSet<QString> paths;
        for (const auto &index : this->selectionModel()->selectedRows()) {
            const auto path = index.data(Qt::UserRole + 1).toString();
            if (!path.isEmpty() && !paths.contains(path)) {
                paths.insert(path);
                urls.push_back(QUrl::fromLocalFile(path));
            }
        }
        if (urls.isEmpty())
            return;
        auto *mime = new QMimeData;
        mime->setUrls(urls);
        QDrag drag(this);
        drag.setMimeData(mime);
        drag.exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
    }
    void dragEnterEvent(QDragEnterEvent *event) override {
        if (event->mimeData()->hasUrls() && acceptsAction(event->dropAction(), event->possibleActions()))
            event->acceptProposedAction();
        else
            event->ignore();
    }
    void dragMoveEvent(QDragMoveEvent *event) override {
        if (event->mimeData()->hasUrls() && acceptsAction(event->dropAction(), event->possibleActions()))
            event->acceptProposedAction();
        else
            event->ignore();
    }
    void dropEvent(QDropEvent *event) override {
        if (!receive || !event->mimeData()->hasUrls()) {
            event->ignore();
            return;
        }
        const auto action = event->dropAction();
        if (!acceptsAction(action, event->possibleActions()) ||
            !receive(event->mimeData(), action, this->indexAt(event->position().toPoint()).row(),
                     event->source() == this)) {
            event->ignore();
            return;
        }
        // The receiver owns filesystem mutation, including source removal. A
        // Copy acknowledgement prevents a second deletion by the sender. On
        // Windows TargetMove explicitly transfers that ownership when Copy is
        // unavailable; other platforms cannot safely acknowledge Move alone.
        event->setDropAction(event->possibleActions().testFlag(Qt::CopyAction) ? Qt::CopyAction
                                                                               : Qt::TargetMoveAction);
        event->accept();
    }

  private:
    static bool acceptsAction(Qt::DropAction action, Qt::DropActions possible) {
        if (action != Qt::CopyAction && action != Qt::MoveAction)
            return false;
        if (!possible.testFlag(action))
            return false;
        if (possible.testFlag(Qt::CopyAction))
            return true;
#ifdef Q_OS_WIN
        return action == Qt::MoveAction;
#else
        return false;
#endif
    }
};
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
    } else if (symbol == "copy") {
        p.drawRoundedRect(QRectF(8, 5, 12, 15), 1, 1);
        p.drawLine(4, 16, 4, 2);
        p.drawLine(4, 2, 15, 2);
    } else if (symbol == "cut") {
        p.drawEllipse(QRectF(3, 14, 6, 6));
        p.drawEllipse(QRectF(15, 14, 6, 6));
        p.drawLine(7, 15, 18, 3);
        p.drawLine(17, 15, 6, 3);
    } else if (symbol == "paste") {
        p.drawRoundedRect(QRectF(5, 5, 14, 16), 1, 1);
        p.drawRoundedRect(QRectF(8, 2, 8, 5), 1, 1);
        p.drawLine(9, 12, 16, 12);
        p.drawLine(9, 16, 15, 16);
    } else if (symbol == "rename") {
        p.drawRoundedRect(QRectF(2, 7, 20, 11), 1, 1);
        p.drawLine(14, 3, 14, 21);
        p.drawLine(11, 3, 17, 3);
        p.drawLine(11, 21, 17, 21);
        p.drawLine(5, 14, 8, 10);
        p.drawLine(8, 10, 11, 14);
    } else if (symbol == "recycle") {
        p.drawLine(4, 6, 20, 6);
        p.drawLine(9, 3, 15, 3);
        p.drawLine(6, 6, 7, 21);
        p.drawLine(7, 21, 17, 21);
        p.drawLine(17, 21, 18, 6);
        p.drawLine(10, 10, 10, 17);
        p.drawLine(14, 10, 14, 17);
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
    } else if (symbol == "home") {
        QPainterPath roof;
        roof.moveTo(3, 11);
        roof.lineTo(12, 3);
        roof.lineTo(21, 11);
        p.drawPath(roof);
        p.drawRoundedRect(QRectF(6, 10, 12, 11), 1, 1);
        p.drawRect(QRectF(10, 15, 4, 6));
    } else if (symbol == "downloads") {
        p.drawLine(12, 3, 12, 16);
        p.drawLine(7, 11, 12, 16);
        p.drawLine(17, 11, 12, 16);
        p.drawLine(5, 17, 5, 21);
        p.drawLine(5, 21, 19, 21);
        p.drawLine(19, 21, 19, 17);
    } else if (symbol == "desktop") {
        p.drawRoundedRect(QRectF(3, 4, 18, 13), 1, 1);
        p.drawLine(12, 17, 12, 21);
        p.drawLine(7, 21, 17, 21);
    } else if (symbol == "pictures") {
        p.drawRoundedRect(QRectF(3, 4, 18, 16), 1, 1);
        p.drawEllipse(QPointF(8, 9), 1.5, 1.5);
        p.drawLine(4, 18, 11, 11);
        p.drawLine(11, 11, 17, 18);
        p.drawLine(14, 15, 18, 11);
        p.drawLine(18, 11, 21, 15);
    } else if (symbol == "search") {
        p.drawEllipse(QRectF(4, 3, 12, 12));
        p.drawLine(14, 14, 21, 21);
    } else if (symbol == "file") {
        p.setBrush(QColor("#d4d8dd"));
        p.setPen(QPen(QColor("#eeeeee"), 1));
        p.drawRoundedRect(QRectF(6, 3, 12, 18), 1, 1);
        p.setPen(QPen(QColor("#647180"), 1.2));
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

// Keep the editable, accessible value as the complete caller path. The unfocused
// presentation displays clickable ancestors, each retaining its full path.
class AddressEdit final : public QLineEdit {
  public:
    using QLineEdit::QLineEdit;
    std::function<void(const QString &)> navigate;
    void setLocation(const Location &location) {
        committed_ = location.kind == Location::Kind::Drives ? QStringLiteral("This PC") : location.path;
        parts_.clear();
        if (location.kind == Location::Kind::Shell) {
            parts_.push_back({location.label, location.path});
            update();
            return;
        }
        QString current = committed_;
        while (!current.isEmpty() && parts_.size() < 256) {
            const QDir directory(current);
            const auto label = directory.dirName();
            parts_.prepend({directory.isRoot() ? QDir::toNativeSeparators(current) : label, current});
            const auto parent = directory.absoluteFilePath(QStringLiteral(".."));
            const auto cleaned = QDir::cleanPath(parent);
            if (location.kind == Location::Kind::Drives || directory.isRoot() ||
                cleaned == QDir::cleanPath(current))
                break;
            current = cleaned;
        }
        setToolTip(committed_);
        update();
    }

  protected:
    void paintEvent(QPaintEvent *event) override {
        hits_.clear();
        if (hasFocus() || text() != committed_ || parts_.isEmpty()) {
            QLineEdit::paintEvent(event);
            return;
        }
        QStyleOptionFrame option;
        initStyleOption(&option);
        QPainter painter(this);
        style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, this);
        lineIcon("drive").paint(&painter, QRect(12, (height() - 18) / 2, 18, 18));
        const auto metrics = painter.fontMetrics();
        int start = 0, used = 42;
        for (const auto &part : parts_)
            used += metrics.horizontalAdvance(part.first) + 27;
        while (start + 1 < parts_.size() && used > width() - 24) {
            used -= metrics.horizontalAdvance(parts_[start].first) + 27;
            ++start;
        }
        int x = 42;
        if (start) {
            painter.setPen(QColor("#b7b7b7"));
            painter.drawText(QRect(x, 0, 26, height()), Qt::AlignCenter, QStringLiteral("…"));
            x += 30;
        }
        for (int i = start; i < parts_.size(); ++i) {
            const int available = std::max(0, width() - x - 12);
            const int labelWidth = std::min(metrics.horizontalAdvance(parts_[i].first) + 12, available);
            const QRect bounds(x, 2, labelWidth, height() - 4);
            painter.setPen(QColor("#e8e8e8"));
            painter.drawText(bounds, Qt::AlignVCenter | Qt::AlignLeft,
                             metrics.elidedText(parts_[i].first, Qt::ElideMiddle, labelWidth));
            hits_.push_back({bounds, parts_[i].second});
            x += labelWidth;
            if (i + 1 < parts_.size()) {
                painter.setPen(QColor("#aaaaaa"));
                painter.drawText(QRect(x, 0, 18, height()), Qt::AlignCenter, QStringLiteral("›"));
                x += 18;
            }
        }
    }
    void mousePressEvent(QMouseEvent *event) override {
        // Qt gives a widget mouse focus before dispatching mousePressEvent.
        // The hit map belongs to the last breadcrumb paint, not hasFocus().
        if (event->button() == Qt::LeftButton && text() == committed_)
            for (const auto &hit : hits_)
                if (hit.first.contains(event->position().toPoint()) && navigate) {
                    clearFocus();
                    navigate(hit.second);
                    event->accept();
                    return;
                }
        QLineEdit::mousePressEvent(event);
    }
    void focusInEvent(QFocusEvent *event) override {
        if (event->reason() != Qt::MouseFocusReason)
            hits_.clear();
        QLineEdit::focusInEvent(event);
    }

  private:
    QString committed_;
    QVector<QPair<QString, QString>> parts_;
    QVector<QPair<QRect, QString>> hits_;
};
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
namespace {
QString thumbnailKey(const Entry &entry) {
    return entry.path + QChar('\n') + entry.identity + QChar('\n') +
        QString::number(entry.modifiedMs.value_or(0)) + ':' + QString::number(entry.sizeBytes.value_or(0));
}
QImage imageThumbnail(const Entry &entry, int edge) {
    if (entry.directory || entry.link || entry.shell) return {};
    QFileInfo info(entry.path);
    if (!info.isFile() || info.size() > 32 * 1024 * 1024) return {};
    QImageReader reader(entry.path);
    const auto size = reader.size();
    if (!size.isValid() || qint64(size.width()) * size.height() > 32000000) return {};
    reader.setAutoTransform(true);
    reader.setScaledSize(size.scaled(edge, edge, Qt::KeepAspectRatio));
    return reader.read();
}
}
EntryModel::EntryModel(QObject *parent) : QAbstractTableModel(parent) {
    thumbnailPool_.setMaxThreadCount(2);
}
EntryModel::~EntryModel() {
    thumbnailPool_.waitForDone();
}
void EntryModel::setThumbnailsEnabled(bool enabled) {
    thumbnailsEnabled_ = enabled;
    if (rowCount()) emit dataChanged(index(0,0), index(rowCount()-1,0), {Qt::DecorationRole});
}
void EntryModel::requestThumbnail(const Entry &entry) {
    const auto key = thumbnailKey(entry);
    if (thumbnailPending_.contains(key) || thumbnailPending_.size() >= 8) return;
    thumbnailPending_.insert(key);
    const auto provider = thumbnailProvider;
    auto *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher, key] {
        const auto image = watcher->result();
        watcher->deleteLater();
        thumbnailPending_.remove(key);
        auto *pixmap = new QPixmap(QPixmap::fromImage(image));
        const int cost = qMax(1, int(image.sizeInBytes() / 1024));
        thumbnails_.insert(key, pixmap, cost);
        if (rowCount()) emit dataChanged(index(0,0), index(rowCount()-1,0), {Qt::DecorationRole});
    });
    watcher->setFuture(QtConcurrent::run(&thumbnailPool_, [entry, provider] {
        auto image = imageThumbnail(entry, 128);
        if (image.isNull() && provider) image = provider(entry, 128);
        return image;
    }));
}
QVariant EntryModel::data(const QModelIndex &index, int role) const {
    const auto *value = entry(index.row());
    if (!value || !index.isValid())
        return {};
    if (role == Qt::UserRole)
        return value->key;
    if (role == Qt::UserRole + 1)
        return value->shell ? QVariant() : QVariant(value->path);
#ifdef Q_OS_WIN
    // Shell type names follow the host language. An explicit system Hangul
    // family avoids the measured first-paint fallback scan in this column.
    if (role == Qt::FontRole && index.column() == 2)
        return QFont(QStringLiteral("Malgun Gothic"));
#endif
    if (role == Qt::DecorationRole && index.column() == 0) {
        if (thumbnailsEnabled_) {
            if (const auto *image = thumbnails_.object(thumbnailKey(*value))) {
                if (!image->isNull()) return QIcon(*image);
            } else {
                const_cast<EntryModel *>(this)->requestThumbnail(*value);
            }
        }
        return lineIcon(value->directory ? "folder" : "file", QColor("#dddddd"), 96);
    }
    if (role == Qt::TextAlignmentRole && index.column() == 3)
        return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
    if (role == Qt::ToolTipRole)
        return value->path + (value->link ? QStringLiteral("\nSymbolic link / reparse point") : QString());
    if (role == Qt::EditRole && index.column() == 0)
        return value->name;
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
bool EntryModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    const auto *item = entry(index.row());
    if (!index.isValid() || index.column() != 0 || role != Qt::EditRole || !item || !renameEntry)
        return false;
    const auto name = value.toString();
    if (name == item->name)
        return true;
    const auto source = *item;
    return renameEntry(source, name);
}
Qt::ItemFlags EntryModel::flags(const QModelIndex &index) const {
    auto flags = QAbstractTableModel::flags(index);
    if (index.isValid()) {
        flags |= Qt::ItemIsDragEnabled;
        if (index.column() == 0 && renameEntry)
            flags |= Qt::ItemIsEditable;
    }
    return flags;
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
    previewPool_.setMaxThreadCount(1);
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
    auto *breadcrumb = new AddressEdit(navigation);
    address_ = breadcrumb;
    breadcrumb->navigate = [this](const QString &path) { session_->navigate(currentTabId(), path); };
    address_->setObjectName("address");
    address_->setAccessibleName("Address");
    address_->setClearButtonEnabled(false);
    address_->setMinimumWidth(250);
    nav->addWidget(address_, 1);
    search_ = new QLineEdit(navigation);
    search_->setObjectName("search");
    search_->setAccessibleName("Search this folder");
    search_->setPlaceholderText("Search this folder");
    search_->setClearButtonEnabled(true);
    search_->addAction(lineIcon("search"), QLineEdit::TrailingPosition);
    search_->setFixedWidth(265);
    nav->addWidget(search_);
    layout->addWidget(navigation);
    auto *commands = new QWidget(root);
    commands->setObjectName("commands");
    auto *commandLayout = new QHBoxLayout(commands);
    commandLayout->setContentsMargins(14, 4, 14, 6);
    commandLayout->setSpacing(6);
    auto *fresh = makeButton("plus", "New folder", commands);
    newFolder_ = fresh;
    newFolder_->setObjectName("newFolder");
    fresh->setText("New");
    fresh->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    fresh->setFixedWidth(92);
    fresh->setEnabled(false);
    commandLayout->addWidget(fresh);
    QVector<QToolButton *> operationButtons;
    for (const auto &pair : QVector<QPair<QString, QString>>{{"cut", "Cut"},
                                                             {"copy", "Copy"},
                                                             {"paste", "Paste"},
                                                             {"rename", "Rename"},
                                                             {"recycle", "Recycle"}}) {
        auto *button = makeButton(pair.first, pair.second, commands);
        button->setObjectName(pair.first);
        operationButtons.push_back(button);
        button->setEnabled(false);
        commandLayout->addWidget(button);
    }
    cut_ = operationButtons[0];
    copy_ = operationButtons[1];
    paste_ = operationButtons[2];
    rename_ = operationButtons[3];
    recycle_ = operationButtons[4];
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
    auto *operationRow = new QWidget(root);
    operationRow->setObjectName("operationRow");
    auto *operationLayout = new QHBoxLayout(operationRow);
    operationLayout->setContentsMargins(14, 3, 14, 3);
    operationLabel_ = new QLabel(operationRow);
    operationLabel_->setObjectName("operationStatus");
    operationLabel_->setWordWrap(true);
    operationLayout->addWidget(operationLabel_, 1);
    operationProgress_ = new QProgressBar(operationRow);
    operationProgress_->setObjectName("operationProgress");
    operationProgress_->setMaximumWidth(170);
    operationLayout->addWidget(operationProgress_);
    cancelOperation_ = makeButton("close", "Cancel operation", operationRow);
    cancelOperation_->setObjectName("cancelOperation");
    operationLayout->addWidget(cancelOperation_);
    retryCleanup_ = makeButton("refresh", "Retry owned cleanup", operationRow);
    retryCleanup_->setObjectName("retryCleanup");
    operationLayout->addWidget(retryCleanup_);
    keepBoth_ = makeButton("copy", "Retry conflicts keeping both", operationRow);
    keepBoth_->setObjectName("keepBoth");
    keepBoth_->setText("Keep both");
    keepBoth_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    keepBoth_->setFixedWidth(115);
    operationLayout->addWidget(keepBoth_);
    operationRow->hide();
    layout->addWidget(operationRow);
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
    const auto addPlace = [this](const QString &name, const QString &symbol,
                                 QStandardPaths::StandardLocation location, const QColor &color) {
        const auto path = QStandardPaths::writableLocation(location);
        if (path.isEmpty())
            return;
        auto *place = new QTreeWidgetItem(tree_, QStringList{name});
        place->setIcon(0, lineIcon(symbol, color));
        place->setData(0, Qt::UserRole + 1, path);
        place->setToolTip(0, path);
    };
    addPlace("Home", "home", QStandardPaths::HomeLocation, QColor("#65b5ee"));
    addPlace("Desktop", "desktop", QStandardPaths::DesktopLocation, QColor("#6ebcff"));
    addPlace("Downloads", "downloads", QStandardPaths::DownloadLocation, QColor("#7ad4be"));
    addPlace("Documents", "file", QStandardPaths::DocumentsLocation, QColor("#82b8ec"));
    addPlace("Pictures", "pictures", QStandardPaths::PicturesLocation, QColor("#b5a4e7"));
    favoritesRoot_ = new QTreeWidgetItem(tree_, QStringList{"Favorites"});
    favoritesRoot_->setIcon(0, lineIcon("folder"));
    favoritesRoot_->setExpanded(true);
    favorites_ = QSettings().value("favorites").toStringList();
    rebuildFavorites();
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tree_, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        auto *item = tree_->itemAt(pos);
        if (!item || item->parent() != favoritesRoot_)
            return;
        const auto path = item->data(0, Qt::UserRole + 1).toString();
        QMenu menu(this);
        auto *remove = menu.addAction("Remove from favorites");
        if (menu.exec(tree_->viewport()->mapToGlobal(pos)) == remove) {
            favorites_.removeAll(path);
            QSettings().setValue("favorites", favorites_);
            rebuildFavorites();
        }
    });
    if (platform_->capabilities().shellLocations) {
        auto *shellRoot = new QTreeWidgetItem(tree_, QStringList{"Windows Shell"});
        for (const auto &place : QVector<QPair<QString, QString>>{
                 {"Desktop namespace", "shell:Desktop"}, {"Recycle Bin", "shell:RecycleBinFolder"},
                 {"Network", "shell:NetworkPlacesFolder"}, {"Libraries", "shell:Libraries"},
                 {"Control Panel", "shell:ControlPanelFolder"}, {"Applications", "shell:AppsFolder"}}) {
            auto *item = new QTreeWidgetItem(shellRoot, QStringList{place.first});
            item->setData(0, Qt::UserRole + 1, place.second);
            item->setIcon(0, lineIcon("folder"));
            new QTreeWidgetItem(item, QStringList{"…"});
        }
        shellRoot->setExpanded(true);
    }
    auto *computer = new QTreeWidgetItem(tree_, QStringList{"This PC"});
    computer->setIcon(0, lineIcon("drive"));
    computer->setData(0, Qt::UserRole, QStringLiteral("drives"));
    computer->setData(0, Qt::UserRole + 1, QStringLiteral("drives"));
    new QTreeWidgetItem(computer, QStringList{"Loading…"});
    views_ = new QStackedWidget(splitter_);
    views_->setObjectName("fileViews");
    auto *transferTable = new TransferView<QTableView>(views_);
    table_ = transferTable;
    table_->setObjectName("fileList");
    table_->setAccessibleName("Files");
    model_ = new EntryModel(this);
    model_->thumbnailProvider = [adapter = platform_](const Entry &entry, int edge) {
        return adapter->thumbnail(entry, edge);
    };
    table_->setModel(model_);
    model_->renameEntry = [this](const Entry &entry, const QString &name) {
        const auto *state = session_->tab(currentTabId());
        if (!state || state->loading || state->location.kind != Location::Kind::Directory ||
            session_->operationState().running || !platform_->capabilities().rename)
            return false;
        OperationRequest request;
        request.kind = OperationKind::Rename;
        request.sources = {entry};
        auto parent = platform_->parseLocation(QFileInfo(entry.path).absolutePath());
        if (const auto *error = failure(parent)) {
            QMessageBox::warning(this, "Rename", error->message);
            return false;
        }
        request.destination = std::get<Location>(parent);
        request.name = name;
        return startOperation(std::move(request));
    };
    icons_ = new TransferView<QListView>(views_);
    icons_->setObjectName("iconList");
    icons_->setAccessibleName("Files icon or list view");
    icons_->setModel(model_);
    icons_->setModelColumn(0);
    icons_->setSelectionModel(table_->selectionModel());
    icons_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    icons_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    icons_->setResizeMode(QListView::Adjust);
    icons_->setUniformItemSizes(true);
    icons_->setLayoutMode(QListView::Batched);
    icons_->setBatchSize(128);
    icons_->setMovement(QListView::Static);
    views_->addWidget(table_);
    views_->addWidget(icons_);
    for (auto *fileView : QVector<QAbstractItemView *>{table_, icons_}) {
        fileView->setDragEnabled(true);
        fileView->setAcceptDrops(true);
        fileView->setDropIndicatorShown(true);
        fileView->setDragDropMode(QAbstractItemView::DragDrop);
    }
    transferTable->receive = [this](const QMimeData *mime, Qt::DropAction action, int row, bool internal) {
        return acceptTransfer(mime, action, row, internal);
    };
    static_cast<TransferView<QListView> *>(icons_)->receive = transferTable->receive;
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setShowGrid(false);
    table_->setAlternatingRowColors(false);
    table_->verticalHeader()->hide();
    table_->verticalHeader()->setMinimumSectionSize(22);
    table_->verticalHeader()->setDefaultSectionSize(22);
    table_->setIconSize(QSize(17, 17));
    table_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table_->horizontalHeader()->setHighlightSections(false);
    table_->horizontalHeader()->setSectionsClickable(true);
    table_->horizontalHeader()->setSortIndicatorShown(true);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table_->setColumnWidth(1, 160);
    table_->setColumnWidth(2, 120);
    table_->setColumnWidth(3, 80);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->setMinimumWidth(410);
    details_ = new QWidget(splitter_);
    details_->setObjectName("detailsPane");
    details_->setMinimumWidth(210);
    auto *detailLayout = new QVBoxLayout(details_);
    detailLayout->setContentsMargins(22, 32, 18, 20);
    auto *folderImage = new QLabel(details_);
    detailImage_ = folderImage;
    folderImage->setPixmap(lineIcon("folder", QColor("#dddddd"), 96).pixmap(96, 96));
    folderImage->setMinimumHeight(165);
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
    previewText_ = new QPlainTextEdit(details_);
    previewText_->setReadOnly(true);
    previewText_->setMaximumHeight(240);
    previewText_->setObjectName("textPreview");
    previewText_->hide();
    detailLayout->addWidget(previewText_);
    detailLayout->addStretch();
    splitter_->addWidget(tree_);
    splitter_->addWidget(views_);
    splitter_->addWidget(details_);
    splitter_->setStretchFactor(1, 1);
    splitter_->setSizes({175, 765, 300});
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
QMainWindow,QWidget{background:#191919;color:#ededed;font-family:"Segoe UI";font-size:13px;}
QLabel{background:transparent;}
QWidget#tabRow{background:#11181e;} QTabBar::tab{background:#171d23;color:#bdbdbd;min-width:190px;max-width:230px;padding:8px 14px;border-top-left-radius:8px;border-top-right-radius:8px;margin-right:3px;} QTabBar::tab:selected{background:#242424;color:white;}
QWidget#navigation{background:#202020;} QLineEdit{background:#2b2c30;border:1px solid transparent;border-radius:5px;padding:8px 10px;selection-background-color:#376899;} QLineEdit:focus{border-bottom:2px solid #75baff;}
QWidget#commands{border-bottom:1px solid #343434;} QToolButton{border:0;border-radius:4px;padding:4px;color:#e0e0e0;} QToolButton:hover{background:#353535;}QToolButton:pressed{background:#414141;}QToolButton:disabled{color:#696969;}
QTreeWidget{border:0;border-right:1px solid #333;padding-top:8px;background:#202020;outline:0;}QTreeWidget::item{padding:5px 4px;border-radius:4px;}QTreeWidget::item:selected{background:#3a3a3a;}QTreeWidget::item:hover{background:#303030;}
QTableView{border:0;background:#191919;selection-background-color:#344454;selection-color:white;outline:0;}QTableView::item{padding-left:7px;border:0;}QTableView::item:selected{background:#354756;} QHeaderView::section{background:#191919;color:#c8c8c8;border:0;border-right:1px solid #343434;border-bottom:1px solid #333;padding:7px 9px;text-align:left;}QHeaderView{background:#191919;}
QListView{border:0;background:#191919;outline:0;}QListView::item{border:0;border-radius:4px;padding:3px;}QListView::item:selected{background:#354756;}QListView::item:hover{background:#303030;}
QWidget#detailsPane{background:#202020;border-left:1px solid #333;}QStatusBar{background:#202020;border-top:1px solid #303030;}QStatusBar::item{border:0;}QLabel#errorBanner{background:#493b26;color:#ffe0ab;padding:10px 16px;border-bottom:1px solid #6a5437;}
QSplitter::handle{background:#303030;width:1px;}QScrollBar:vertical{background:#1c1c1c;width:10px;}QScrollBar::handle:vertical{background:#555;border-radius:4px;min-height:25px;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
QMenu{background:#292929;border:1px solid #454545;padding:6px;}QMenu::item{padding:7px 26px;}QMenu::item:selected{background:#414141;border-radius:3px;}
)");
    connect(session_, &ExplorerSession::tabsChanged, this, &MainWindow::syncTabs);
    connect(session_, &ExplorerSession::tabChanged, this, &MainWindow::render);
    connect(session_, &ExplorerSession::shutdownFinished, this, &MainWindow::finishClose);
    connect(session_, &ExplorerSession::operationChanged, this, &MainWindow::renderOperation);
    connect(cancelOperation_, &QToolButton::clicked, session_, &ExplorerSession::cancelOperation);
    connect(retryCleanup_, &QToolButton::clicked, this, [this] {
        const auto state = session_->operationState();
        if (state.report && !state.report->cleanupPending.isEmpty())
            session_->retryOperationCleanup(state.report->cleanupPending.first());
    });
    connect(keepBoth_, &QToolButton::clicked, this, [this] {
        const auto state = session_->operationState();
        if (!state.report)
            return;
        auto request = state.request;
        request.sources.clear();
        for (const auto &item : state.report->items)
            if (item.error && item.error->code == ErrorCode::Conflict)
                for (const auto &source : state.request.sources)
                    if (source.path == item.sourcePath)
                        request.sources.push_back(source);
        request.conflict = ConflictPolicy::KeepBoth;
        if (!request.sources.isEmpty())
            startOperation(std::move(request));
    });
    connect(newFolder_, &QToolButton::clicked, this, &MainWindow::createDirectory);
    connect(cut_, &QToolButton::clicked, this, [this] { copySelection(true); });
    connect(copy_, &QToolButton::clicked, this, [this] { copySelection(false); });
    connect(paste_, &QToolButton::clicked, this, &MainWindow::pasteSelection);
    connect(rename_, &QToolButton::clicked, this, &MainWindow::renameSelection);
    connect(recycle_, &QToolButton::clicked, this, &MainWindow::recycleSelection);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::updateSelection);
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
        if (!updating_) {
            searchTab_ = currentTabId();
            pendingSearch_ = value;
            if (const auto *tab = session_->tab(searchTab_)) searchGeneration_ = tab->generation;
            if (const auto *tab = session_->tab(searchTab_); tab && tab->recursive)
                searchTimer_.start();
            else
                session_->setSearch(searchTab_, value);
        }
    });
    connect(table_, &QTableView::doubleClicked, this, &MainWindow::activateRow);
    connect(icons_, &QListView::doubleClicked, this, &MainWindow::activateRow);
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
        if (path.isEmpty())
            return;
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
    const auto fileShortcut = [this](const QString &key, auto action) {
        auto *value = new QShortcut(QKeySequence(key), views_);
        value->setContext(Qt::WidgetWithChildrenShortcut);
        connect(value, &QShortcut::activated, this, [action] {
            if (qobject_cast<QLineEdit *>(QApplication::focusWidget())) return;
            action();
        });
    };
    fileShortcut("Ctrl+C", [this] { copySelection(false); });
    fileShortcut("Ctrl+X", [this] { copySelection(true); });
    fileShortcut("Ctrl+V", [this] { pasteSelection(); });
    fileShortcut("F2", [this] { renameSelection(); });
    fileShortcut("Delete", [this] { recycleSelection(); });
    fileShortcut("Ctrl+Shift+N", [this] { createDirectory(); });
    fileShortcut("Ctrl+A", [this] { activeView()->selectAll(); });
    shortcut(QKeySequence("Ctrl+Shift+1"), [this] { setViewMode(1); });
    shortcut(QKeySequence("Ctrl+Shift+5"), [this] { setViewMode(2); });
    shortcut(QKeySequence("Ctrl+Shift+6"), [this] { setViewMode(0); });
    auto *openShortcut = new QShortcut(QKeySequence(Qt::Key_Return), table_);
    openShortcut->setContext(Qt::WidgetShortcut);
    connect(openShortcut, &QShortcut::activated, this, [this] { activateRow(table_->currentIndex()); });
    auto *openIcons = new QShortcut(QKeySequence(Qt::Key_Return), icons_);
    openIcons->setContext(Qt::WidgetShortcut);
    connect(openIcons, &QShortcut::activated, this, [this] { activateRow(icons_->currentIndex()); });
    progressTimer_.setInterval(120);
    connect(&progressTimer_, &QTimer::timeout, this, [this] {
        const auto *state = session_->tab(currentTabId());
        if (state && state->loading && state->examined)
            status_->setText(QStringLiteral("Loading… %1 items examined").arg(state->examined->load()));
        if (session_->operationState().running)
            renderOperation();
    });
    progressTimer_.start();
    searchTimer_.setSingleShot(true);
    searchTimer_.setInterval(250);
    connect(&searchTimer_, &QTimer::timeout, this, [this] {
        if (const auto *tab = session_->tab(searchTab_); tab && tab->generation == searchGeneration_)
            session_->setSearch(searchTab_, pendingSearch_);
    });
    auto *searchOptions = search_->addAction(lineIcon("folder"), QLineEdit::LeadingPosition);
    searchOptions->setToolTip("Search scope and filters");
    connect(searchOptions, &QAction::triggered, this, [this] {
        QMenu menu(this);
        auto *recursive = menu.addAction("Search subfolders");
        recursive->setCheckable(true);
        if (const auto *tab = session_->tab(currentTabId())) recursive->setChecked(tab->recursive);
        connect(recursive, &QAction::toggled, this, [this](bool enabled) {
            if (searchTimer_.isActive()) {
                searchTimer_.stop();
                session_->setSearch(searchTab_, pendingSearch_);
            }
            session_->setRecursiveSearch(currentTabId(), enabled);
        });
        connect(menu.addAction("Cancel search"), &QAction::triggered, this,
                [this] { searchTimer_.stop(); session_->cancelSearch(currentTabId()); });
        connect(menu.addAction("Filter examples"), &QAction::triggered, this, [this] {
            QMessageBox::information(this, "Search filters",
                "? ext:png,jpg\n? type:file size:>1mb\n? after:2026-01-01 before:2026-12-31\n? name:\"meeting notes\"\n\nEnable Search subfolders to search the directory tree.");
        });
        menu.exec(search_->mapToGlobal(QPoint(0, search_->height())));
    });
    auto *cancelSearch = new QShortcut(QKeySequence(Qt::Key_Escape), search_);
    cancelSearch->setContext(Qt::WidgetShortcut);
    connect(cancelSearch, &QShortcut::activated, this, [this] { searchTimer_.stop(); session_->cancelSearch(currentTabId()); });
    fileShortcut("Ctrl+Z", [this] { replayOperation(false); });
    fileShortcut("Ctrl+Y", [this] { replayOperation(true); });
    fileShortcut("Ctrl+Shift+Z", [this] { replayOperation(true); });
    for (auto *view : {static_cast<QAbstractItemView *>(table_), static_cast<QAbstractItemView *>(icons_)}) {
        view->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(view, &QWidget::customContextMenuRequested, this,
                [this, view](const QPoint &pos) { showFileMenu(view, pos); });
    }
    search_->setToolTip("Search names, or use ? ext:png type:file size:>1mb after:2026-01-01 name");
    shortcut(QKeySequence("Ctrl+D"), [this] { toggleFavorite(); });
    fileWatcher_ = new QFileSystemWatcher(this);
    watchTimer_.setSingleShot(true);
    watchTimer_.setInterval(350);
    auto changed = [this](const QString &path) {
        changedPaths_.insert(path);
        watchTimer_.start();
    };
    connect(fileWatcher_, &QFileSystemWatcher::directoryChanged, this, changed);
    connect(fileWatcher_, &QFileSystemWatcher::fileChanged, this, changed);
    connect(&watchTimer_, &QTimer::timeout, this, [this] {
        if (closing_)
            return;
        if (session_->operationState().running ||
            (qobject_cast<QLineEdit *>(QApplication::focusWidget()) &&
             views_->isAncestorOf(QApplication::focusWidget()))) {
            watchTimer_.start();
            return;
        }
        const auto paths = std::exchange(changedPaths_, {});
        QVector<quint64> refresh;
        for (const auto &tab : session_->tabs()) {
            bool dirty = false;
            for (const auto &path : paths)
                dirty |= tab.location.path == path || tab.location.path == QFileInfo(path).absolutePath() ||
                         tab.watchedDirectories.contains(path) ||
                         tab.watchedDirectories.contains(QFileInfo(path).absolutePath());
            if (!dirty)
                continue;
            if (tab.loading)
                changedPaths_.unite(paths);
            else
                refresh.push_back(tab.id);
        }
        for (const auto id : refresh)
            session_->refresh(id);
        if (!changedPaths_.isEmpty())
            watchTimer_.start();
    });
    previewTimer_.setSingleShot(true);
    previewTimer_.setInterval(120);
    connect(&previewTimer_, &QTimer::timeout, this, &MainWindow::loadPreview);
    QSettings settings;
    model_->setThumbnailsEnabled(settings.value("window/thumbnails", true).toBool());
    restoreGeometry(settings.value("window/geometry").toByteArray());
    splitter_->restoreState(settings.value("window/splitter").toByteArray());
    table_->horizontalHeader()->restoreState(settings.value("window/header").toByteArray());
    details_->setVisible(settings.value("window/details", true).toBool());
    const int defaultView = qBound(0, settings.value("window/view", 0).toInt(), 2);
    const auto savedTabs = settings.value("tabs").toStringList().mid(0, 20);
    const auto recursiveModes = settings.value("tabOptions/recursive").toList();
    const auto savedViews = settings.value("tabOptions/views").toList();
    int restoredIndex = 0;
    if (savedTabs.isEmpty())
        session_->createTab();
    else
        for (const auto &path : savedTabs) {
            const auto id = session_->createTab();
            setViewMode(restoredIndex < savedViews.size() ? qBound(0, savedViews[restoredIndex].toInt(), 2) : defaultView);
            session_->setRecursiveSearch(id, restoredIndex < recursiveModes.size() && recursiveModes[restoredIndex].toBool());
            ++restoredIndex;
            if (!path.isEmpty() && (path.startsWith("shell:", Qt::CaseInsensitive) || path.startsWith("shell-item:") ||
                                   path.startsWith("::{") || QFileInfo(path).isDir()))
                session_->navigate(id, path);
        }
    setViewMode(savedTabs.isEmpty() ? defaultView : viewModes_.value(currentTabId(), defaultView));
    tabs_->setCurrentIndex(qBound(0, settings.value("window/activeTab", 0).toInt(), tabs_->count() - 1));
    computer->setExpanded(true);
}
MainWindow::~MainWindow() {
    for (const auto &stop : treeStops_)
        stop->request_stop();
    treePool_.waitForDone();
    previewPool_.waitForDone();
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
    QElapsedTimer elapsed;
    elapsed.start();
    const auto *state = session_->tab(id);
    if (!state)
        return;
    for (int i = 0; i < tabs_->count(); ++i)
        if (tabs_->tabData(i).toULongLong() == id)
            tabs_->setTabText(i, state->loading ? QStringLiteral("Loading…") : state->location.label);
    if (fileWatcher_)
        syncWatches();
    if (id != currentTabId())
        return;
    if (viewMode_ != viewModes_.value(id, 0))
        setViewMode(viewModes_.value(id, 0));
    updating_ = true;
    QSignalBlocker searchBlock(search_);
    QSignalBlocker selectionBlock(table_->selectionModel());
    if (!address_->hasFocus() || !state->loading)
        address_->setText(state->error && !state->pendingInput.isEmpty()
                              ? state->pendingInput
                              : (state->location.kind == Location::Kind::Drives ? QStringLiteral("This PC")
                                                                                : state->location.path));
    if (!(searchTimer_.isActive() && searchTab_ == id && searchGeneration_ == state->generation))
        search_->setText(state->query);
    static_cast<AddressEdit *>(address_)->setLocation(state->location);
    search_->setPlaceholderText((state->recursive ? "Search subfolders in " : "Search ") + state->location.label);
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
    error_->setVisible(state->error.has_value() || !state->searchWarnings.isEmpty());
    if (state->error)
        error_->setText(errorCodeName(state->error->code) + QStringLiteral(" — ") + state->error->message +
                        QStringLiteral("  Previous folder remains available. Correct the address or retry."));
    if (!state->error && !state->searchWarnings.isEmpty())
        error_->setText(state->searchWarnings.join("\n"));
    progress_->setVisible(state->loading);
    status_->setText(state->loading ? QStringLiteral("Loading…")
                                    : QStringLiteral("%1 items%2")
                                          .arg(state->visible.size())
                                          .arg(state->query.isEmpty()
                                                   ? QString()
                                                   : QStringLiteral(" matching “%1”").arg(state->query)));
    detailName_->setText(state->location.label);
    detailImage_->setPixmap(lineIcon("folder", QColor("#dddddd"), 96).pixmap(96, 96));
    detailMeta_->setText(
        QStringLiteral("%1 items\n\nSelect a file to see its details.").arg(state->visible.size()));
    setWindowTitle(state->location.label + QStringLiteral(" — REA Explorer"));
    updating_ = false;
    updateSelection();
    lastRenderMilliseconds_ = elapsed.elapsed();
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
    const bool busy = session_->operationState().running;
    const auto caps = platform_->capabilities();
    const bool writable =
        state && !state->loading && state->location.kind == Location::Kind::Directory && !busy;
    newFolder_->setEnabled(writable && caps.mkdir);
    const auto entries = selectedEntries();
    const bool filesystemSelection = !entries.isEmpty() && std::all_of(entries.cbegin(), entries.cend(),
        [](const Entry &entry) { return !entry.shell; });
    cut_->setEnabled(filesystemSelection && !busy && caps.move && writable);
    copy_->setEnabled(filesystemSelection && caps.copy && state && state->location.kind == Location::Kind::Directory);
    rename_->setEnabled(writable && keys.size() == 1 && caps.rename);
    recycle_->setEnabled(filesystemSelection && writable && caps.recycle);
    const auto *mime = QApplication::clipboard()->mimeData();
    paste_->setEnabled(writable && mime && mime->hasUrls() && caps.copy);
    requestPreview(keys.size() == 1 && single && !single->shell ? single : nullptr);
    if (!state || state->loading)
        return;
    if (!keys.isEmpty()) {
        detailImage_->setPixmap(
            lineIcon(keys.size() == 1 && !single->directory ? "file" : "folder", QColor("#dddddd"), 96)
                .pixmap(96, 96));
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
    const QPersistentModelIndex itemIndex(tree_->indexFromItem(item));
    connect(watcher, &QFutureWatcher<Result<Listing>>::finished, this, [this, watcher, key, stop, itemIndex] {
        const auto result = watcher->result();
        watcher->deleteLater();
        --treeActive_;
        treeStops_.remove(key);
        if (!closing_ && !stop->stop_requested()) {
            auto *target = itemIndex.isValid() ? tree_->itemFromIndex(itemIndex) : nullptr;
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
    saveSettings();
    closing_ = true;
    watchTimer_.stop();
    searchTimer_.stop();
    previewTimer_.stop();
    ++previewGeneration_;
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
    const QStringList labels{"Details", "Icons", "List"};
    for (int mode = 0; mode < labels.size(); ++mode) {
        auto *action = menu.addAction(labels[mode]);
        action->setCheckable(true);
        action->setChecked(mode == viewMode_);
        connect(action, &QAction::triggered, this, [this, mode] { setViewMode(mode); });
    }
    menu.addSeparator();
    auto *thumbnails = menu.addAction("File thumbnails");
    thumbnails->setCheckable(true);
    thumbnails->setChecked(QSettings().value("window/thumbnails", true).toBool());
    connect(thumbnails, &QAction::toggled, this, [this](bool enabled) {
        QSettings().setValue("window/thumbnails", enabled);
        model_->setThumbnailsEnabled(enabled);
    });
    auto *details = menu.addAction("Preview and details pane");
    details->setCheckable(true);
    details->setChecked(details_->isVisible());
    connect(details, &QAction::toggled, details_, &QWidget::setVisible);
    menu.exec(QCursor::pos());
}
QAbstractItemView *MainWindow::activeView() const {
    return viewMode_ == 0 ? static_cast<QAbstractItemView *>(table_) : icons_;
}
void MainWindow::setViewMode(int mode) {
    if (mode < 0 || mode > 2)
        return;
    viewMode_ = mode;
    viewModes_.insert(currentTabId(), mode);
    if (mode == 0)
        views_->setCurrentWidget(table_);
    else {
        icons_->setViewMode(mode == 1 ? QListView::IconMode : QListView::ListMode);
        icons_->setIconSize(mode == 1 ? QSize(64, 64) : QSize(22, 22));
        icons_->setGridSize(mode == 1 ? QSize(145, 110) : QSize());
        icons_->setWordWrap(mode == 1);
        icons_->setSpacing(mode == 1 ? 8 : 2);
        views_->setCurrentWidget(icons_);
    }
    activeView()->setFocus();
}
QVector<Entry> MainWindow::selectedEntries() const {
    QVector<Entry> entries;
    for (const auto &index : table_->selectionModel()->selectedRows())
        if (const auto *item = model_->entry(index.row()))
            entries.push_back(*item);
    return entries;
}
void MainWindow::copySelection(bool cut) {
    auto entries = selectedEntries();
    const auto *tab = session_->tab(currentTabId());
    if (entries.isEmpty() || !tab || tab->loading || tab->location.kind != Location::Kind::Directory ||
        std::any_of(entries.cbegin(), entries.cend(), [](const Entry &entry) { return entry.shell; }))
        return;
    clipboardEntries_ = std::move(entries);
    clipboardCut_ = cut;
    clipboardToken_ = QUuid::createUuid().toString();
    auto *mime = new QMimeData;
    QList<QUrl> urls;
    for (const auto &entry : clipboardEntries_)
        urls.push_back(QUrl::fromLocalFile(entry.path));
    mime->setUrls(urls);
    mime->setData("application/x-rea-explorer-clipboard", clipboardToken_.toUtf8());
    // Windows Shell's registered Preferred DropEffect clipboard format.
    QByteArray effect(4, '\0');
    effect[0] = cut ? '\2' : '\1';
    mime->setData("application/x-qt-windows-mime;value=\"Preferred DropEffect\"", effect);
    QApplication::clipboard()->setMimeData(mime);
    updateSelection();
}
void MainWindow::pasteSelection() {
    const auto *state = session_->tab(currentTabId());
    if (!state || state->loading || state->location.kind != Location::Kind::Directory)
        return;
    const auto *mime = QApplication::clipboard()->mimeData();
    if (!mime)
        return;
    if (!clipboardEntries_.isEmpty() &&
        mime->data("application/x-rea-explorer-clipboard") == clipboardToken_.toUtf8()) {
        OperationRequest request;
        request.kind = clipboardCut_ ? OperationKind::Move : OperationKind::Copy;
        request.sources = clipboardEntries_;
        request.destination = state->location;
        startOperation(std::move(request));
    } else {
        const auto effect = mime->data("application/x-qt-windows-mime;value=\"Preferred DropEffect\"");
        const bool move = effect.size() == 4 && effect[0] == '\2' && effect[1] == '\0' && effect[2] == '\0' &&
                          effect[3] == '\0';
        acceptTransfer(mime, move ? Qt::MoveAction : Qt::CopyAction, -1, false);
    }
}
void MainWindow::renameSelection() {
    const auto rows = table_->selectionModel()->selectedRows();
    const auto *state = session_->tab(currentTabId());
    if (rows.size() != 1 || !state || state->loading || !rename_->isEnabled())
        return;
    auto *view = activeView();
    view->setCurrentIndex(rows.first());
    view->setFocus();
    view->edit(rows.first());
}
void MainWindow::createDirectory() {
    const auto *state = session_->tab(currentTabId());
    if (!state || state->loading || state->location.kind != Location::Kind::Directory)
        return;
    const auto destination = state->location;
    bool accepted = false;
    const auto name =
        QInputDialog::getText(this, "New folder", "Folder name", QLineEdit::Normal, "New folder", &accepted);
    if (!accepted)
        return;
    OperationRequest request;
    request.kind = OperationKind::CreateDirectory;
    request.destination = destination;
    request.name = name;
    startOperation(std::move(request));
}
void MainWindow::recycleSelection() {
    const auto entries = selectedEntries();
    const auto *tab = session_->tab(currentTabId());
    if (entries.isEmpty() || !tab || tab->loading || tab->location.kind != Location::Kind::Directory ||
        session_->operationState().running || !platform_->capabilities().recycle ||
        std::any_of(entries.cbegin(), entries.cend(), [](const Entry &entry) { return entry.shell; }))
        return;
    QStringList paths;
    for (const auto &entry : entries)
        paths.push_back(entry.path);
    if (QMessageBox::question(this, "Move to Recycle Bin",
                              QStringLiteral("Move %1 selected items to the OS Recycle Bin?\n\n%2")
                                  .arg(entries.size())
                                  .arg(paths.mid(0, 12).join('\n')),
                              QMessageBox::Yes | QMessageBox::Cancel,
                              QMessageBox::Cancel) != QMessageBox::Yes)
        return;
    OperationRequest request;
    request.kind = OperationKind::Recycle;
    request.sources = entries;
    startOperation(std::move(request));
}
bool MainWindow::startOperation(OperationRequest request) {
    if (!session_->beginOperation(std::move(request))) {
        QMessageBox::information(this, "File operation",
                                 "Wait for the current operation or retry its retained cleanup first.");
        return false;
    }
    return true;
}
bool MainWindow::acceptTransfer(const QMimeData *mime, Qt::DropAction action, int row, bool internal) {
    const auto *state = session_->tab(currentTabId());
    const auto operation = session_->operationState();
    if (!state || state->loading || state->location.kind != Location::Kind::Directory || operation.running ||
        (operation.report && !operation.report->cleanupPending.isEmpty()) || closing_)
        return false;
    Location destination = state->location;
    if (const auto *item = model_->entry(row); item && item->directory && !item->link && !item->shell)
        destination = Location{
            Location::Kind::Directory, item->path, item->path, item->key, item->name, item->identity};
    const auto kind = action == Qt::MoveAction ? OperationKind::Move : OperationKind::Copy;
    if (internal) {
        OperationRequest request;
        request.kind = kind;
        request.destination = destination;
        request.sources = selectedEntries();
        return !request.sources.isEmpty() && startOperation(std::move(request));
    }
    QStringList paths;
    quint64 chars = 0;
    for (const auto &url : mime->urls()) {
        if (!url.isLocalFile()) {
            QMessageBox::warning(this, "Unsupported drop", "Only local filesystem URLs can be transferred.");
            return false;
        }
        const auto path = url.toLocalFile();
        chars += static_cast<quint64>(path.size());
        if (chars > 8 * 1024 * 1024 || paths.size() >= 20000) {
            QMessageBox::warning(this, "Resource limit", "Transfer input exceeds the bounded path budget.");
            return false;
        }
        paths.push_back(path);
    }
    return !paths.isEmpty() && resolveTransfer(std::move(paths), std::move(destination), kind);
}
bool MainWindow::resolveTransfer(QStringList paths, Location destination, OperationKind kind) {
    if (closing_ || treeActive_ >= 8)
        return false;
    const auto adapter = platform_;
    const auto token = QStringLiteral("transfer:") + QUuid::createUuid().toString();
    auto stop = std::make_shared<std::stop_source>();
    treeStops_.insert(token, stop);
    ++treeActive_;
    auto *watcher = new QFutureWatcher<Result<QVector<Entry>>>(this);
    connect(watcher, &QFutureWatcher<Result<QVector<Entry>>>::finished, this,
            [this, watcher, destination, kind, token] {
                auto result = watcher->result();
                watcher->deleteLater();
                --treeActive_;
                treeStops_.remove(token);
                if (!closing_) {
                    if (const auto *error = failure(result))
                        QMessageBox::warning(this, errorCodeName(error->code), error->message);
                    else {
                        OperationRequest request;
                        request.kind = kind;
                        request.destination = destination;
                        request.sources = std::get<QVector<Entry>>(std::move(result));
                        startOperation(std::move(request));
                    }
                }
                finishClose();
            });
    watcher->setFuture(
        QtConcurrent::run(&treePool_, [adapter, paths = std::move(paths), stop]() -> Result<QVector<Entry>> {
            QVector<Entry> entries;
            for (const auto &path : paths) {
                if (stop->stop_requested())
                    return Error{ErrorCode::Cancelled, QStringLiteral("resolve transfer"), path,
                                 QStringLiteral("Transfer cancelled before execution")};
                auto item = adapter->inspectEntry(path);
                if (const auto *error = failure(item))
                    return *error;
                entries.push_back(std::get<Entry>(std::move(item)));
            }
            return entries;
        }));
    return true;
}
void MainWindow::renderOperation() {
    const auto state = session_->operationState();
    auto *row = findChild<QWidget *>("operationRow");
    row->setVisible(state.running || state.report.has_value());
    cancelOperation_->setVisible(state.running);
    retryCleanup_->setVisible(!state.running && state.report && !state.report->cleanupPending.isEmpty());
    keepBoth_->hide();
    operationProgress_->setVisible(state.running);
    if (state.running) {
        operationLabel_->setText(QStringLiteral("%1 · %2/%3 items · %4 copied\n%5")
                                     .arg(operationName(state.request.kind))
                                     .arg(state.progress.completedItems)
                                     .arg(state.progress.totalItems)
                                     .arg(sizeText(state.progress.completedBytes))
                                     .arg(state.progress.currentPath));
        if (state.progress.totalBytes && *state.progress.totalBytes) {
            operationProgress_->setRange(0, 1000);
            operationProgress_->setValue(
                static_cast<int>(1000.0 * static_cast<double>(state.progress.completedBytes) /
                                 static_cast<double>(*state.progress.totalBytes)));
        } else
            operationProgress_->setRange(0, 0);
    } else if (state.report) {
        int succeeded = 0, failed = 0, cancelled = 0;
        QStringList errors;
        bool conflict = false;
        for (const auto &item : state.report->items) {
            if (item.status == ItemStatus::Succeeded)
                ++succeeded;
            else if (item.status == ItemStatus::Cancelled)
                ++cancelled;
            else
                ++failed;
            if (item.error) {
                errors.push_back(item.sourcePath + ": " + item.error->message);
                conflict |= item.error->code == ErrorCode::Conflict;
            }
        }
        operationLabel_->setText(QStringLiteral("%1: %2 succeeded, %3 failed/partial, %4 cancelled%5")
                                     .arg(operationName(state.request.kind))
                                     .arg(succeeded)
                                     .arg(failed)
                                     .arg(cancelled)
                                     .arg(state.report->cleanupPending.isEmpty()
                                              ? QString()
                                              : QStringLiteral(" · Cleanup retained; retry required")));
        operationLabel_->setToolTip(errors.join('\n'));
        keepBoth_->setVisible(
            conflict &&
            (state.request.kind == OperationKind::Copy || state.request.kind == OperationKind::Move) &&
            state.report->cleanupPending.isEmpty());
        if (!state.report->historyWarning.isEmpty())
            errors.push_back(state.report->historyWarning);
        if (!errors.isEmpty())
            operationLabel_->setText(operationLabel_->text() + "\n" + errors.first());
        if (clipboardCut_ && state.request.kind == OperationKind::Move) {
            const auto *mime = QApplication::clipboard()->mimeData();
            if (mime && mime->data("application/x-rea-explorer-clipboard") == clipboardToken_.toUtf8()) {
                QSet<QString> moved;
                for (const auto &item : state.report->items)
                    if (item.status == ItemStatus::Succeeded)
                        moved.insert(item.sourcePath);
                clipboardEntries_.erase(
                    std::remove_if(clipboardEntries_.begin(), clipboardEntries_.end(),
                                   [&](const auto &entry) { return moved.contains(entry.path); }),
                    clipboardEntries_.end());
                if (clipboardEntries_.isEmpty() && !moved.isEmpty()) {
                    clipboardCut_ = false;
                    QApplication::clipboard()->clear();
                }
            }
        }
    }
    updateSelection();
}
void MainWindow::showNativeMenu(QVector<Entry> entries, Location location, QPoint position) {
    if (session_->operationState().running)
        return;
    const auto result = platform_->showContextMenu(location, entries, quintptr(winId()), position);
    if (const auto *error = failure(result))
        QMessageBox::warning(this, "Windows context menu", error->message);
    QVector<quint64> ids;
    for (const auto &tab : session_->tabs()) ids.push_back(tab.id);
    for (auto id : ids) session_->refresh(id);
}
void MainWindow::replayOperation(bool redo) {
    if (session_->operationState().running || closing_) return;
    const auto history = platform_->operationHistory();
    if (redo ? !history.canRedo : !history.canUndo) return;
    OperationRequest request;
    request.kind = redo ? OperationKind::Redo : OperationKind::Undo;
    startOperation(std::move(request));
}
void MainWindow::showFileMenu(QAbstractItemView *view, const QPoint &position) {
    const auto index = view->indexAt(position);
    if (index.isValid() && !view->selectionModel()->isSelected(index)) {
        view->selectionModel()->setCurrentIndex(index,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        updateSelection();
    } else if (!index.isValid()) {
        view->clearSelection();
    }
    const auto entries = selectedEntries();
    const auto *state = session_->tab(currentTabId());
    if (!state)
        return;
    const auto location = state->location;
    const auto point = view->viewport()->mapToGlobal(position);
    const bool busy = session_->operationState().running;
    if (platform_->capabilities().nativeMenu && (QApplication::keyboardModifiers() & Qt::ShiftModifier)) {
        showNativeMenu(entries, location, point);
        return;
    }
    QMenu menu(this);
    if (platform_->capabilities().nativeMenu) {
        auto *native = menu.addAction("Windows context menu… (Shift+right-click)");
        native->setEnabled(!busy);
        connect(native, &QAction::triggered, this, [this, entries, location, point] {
            QTimer::singleShot(0, this, [this, entries, location, point] {
                showNativeMenu(entries, location, point);
            });
        });
        menu.addSeparator();
    }
    const auto history = busy ? OperationHistory{} : platform_->operationHistory();
    auto *undo = menu.addAction("Undo " + history.undoLabel + "\tCtrl+Z");
    auto *redo = menu.addAction("Redo " + history.redoLabel + "\tCtrl+Y");
    undo->setEnabled(history.canUndo && !busy);
    redo->setEnabled(history.canRedo && !busy);
    connect(undo, &QAction::triggered, this, [this] { replayOperation(false); });
    connect(redo, &QAction::triggered, this, [this] { replayOperation(true); });
    menu.addSeparator();
    if (!entries.isEmpty()) {
        auto *open = menu.addAction("Open");
        open->setEnabled(entries.size() == 1);
        connect(open, &QAction::triggered, this, [this, entries] {
            session_->openEntry(currentTabId(), entries.first().key);
        });
        if (entries.size() == 1 && entries.first().directory) {
            const auto path = entries.first().path;
            connect(menu.addAction("Open in new tab"), &QAction::triggered, this, [this, path] {
                const auto id = session_->createTab();
                session_->navigate(id, path);
            });
        }
        if (entries.size() == 1 && state->recursiveResults) {
            const auto parent = QFileInfo(entries.first().path).absolutePath();
            connect(menu.addAction("Open containing folder in new tab"), &QAction::triggered, this,
                [this, parent] {
                    const auto id = session_->createTab();
                    session_->navigate(id, parent);
                });
        }
        menu.addSeparator();
        auto addCommand = [&menu](const QString &label, QToolButton *button) {
            auto *action = menu.addAction(label);
            action->setEnabled(button->isEnabled());
            QObject::connect(action, &QAction::triggered, button, &QToolButton::click);
        };
        addCommand("Cut", cut_);
        addCommand("Copy", copy_);
        addCommand("Rename", rename_);
        addCommand("Move to Recycle Bin", recycle_);
        connect(menu.addAction("Copy paths"), &QAction::triggered, this, [entries] {
            QStringList paths;
            for (const auto &entry : entries)
                paths.push_back(entry.path);
            QApplication::clipboard()->setText(paths.join('\n'));
        });
        connect(menu.addAction("Properties"), &QAction::triggered, this, [this, entries] {
            QStringList text;
            for (const auto &entry : entries.mid(0, 20))
                text << entry.name + "\n" + entry.path + "\n" + entry.typeName + "\n" +
                        sizeText(entry.sizeBytes) + "\nModified: " +
                        (entry.modifiedMs ? QDateTime::fromMSecsSinceEpoch(*entry.modifiedMs).toString(Qt::ISODate)
                                          : QStringLiteral("Unknown"));
            QMessageBox::information(this, "Properties", text.join("\n\n"));
        });
    } else {
        auto *paste = menu.addAction("Paste");
        paste->setEnabled(paste_->isEnabled());
        connect(paste, &QAction::triggered, paste_, &QToolButton::click);
        auto *folder = menu.addAction("New folder");
        folder->setEnabled(newFolder_->isEnabled());
        connect(folder, &QAction::triggered, newFolder_, &QToolButton::click);
        connect(menu.addAction("Refresh"), &QAction::triggered, this,
                [this] { session_->refresh(currentTabId()); });
    }
    menu.addSeparator();
    auto *favorite = menu.addAction(favorites_.contains(location.path)
                                       ? "Remove current folder from favorites"
                                       : "Add current folder to favorites (Ctrl+D)");
    favorite->setEnabled(location.kind != Location::Kind::Drives);
    connect(favorite, &QAction::triggered, this, &MainWindow::toggleFavorite);
    menu.exec(view->viewport()->mapToGlobal(position));
}
void MainWindow::toggleFavorite() {
    const auto *state = session_->tab(currentTabId());
    if (!state || state->loading || state->location.kind == Location::Kind::Drives)
        return;
    const auto path = state->location.kind == Location::Kind::Shell ? state->location.path : QDir::cleanPath(state->location.path);
    if (favorites_.contains(path))
        favorites_.removeAll(path);
    else
        favorites_.push_back(path);
    QSettings settings;
    settings.setValue("favorites", favorites_);
    auto labels = settings.value("favoriteLabels").toMap();
    if (favorites_.contains(path)) labels.insert(path, state->location.label);
    else labels.remove(path);
    settings.setValue("favoriteLabels", labels);
    rebuildFavorites();
}
void MainWindow::rebuildFavorites() {
    qDeleteAll(favoritesRoot_->takeChildren());
    const auto labels = QSettings().value("favoriteLabels").toMap();
    for (const auto &path : favorites_) {
        const auto name = labels.value(path, QFileInfo(path).fileName()).toString();
        auto *item = new QTreeWidgetItem(favoritesRoot_, QStringList{name.isEmpty() ? path : name});
        item->setData(0, Qt::UserRole + 1, path);
        item->setToolTip(0, path);
        item->setIcon(0, lineIcon("folder"));
    }
    favoritesRoot_->setExpanded(true);
}
void MainWindow::saveSettings() {
    QSettings settings;
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/splitter", splitter_->saveState());
    settings.setValue("window/header", table_->horizontalHeader()->saveState());
    settings.setValue("window/details", details_->isVisible());
    settings.setValue("window/view", viewMode_);
    settings.setValue("window/activeTab", tabs_->currentIndex());
    QStringList paths;
    QVariantList recursiveModes, views;
    for (const auto &tab : session_->tabs()) {
        paths.push_back(tab.location.kind != Location::Kind::Drives ? tab.location.path : QString());
        recursiveModes.push_back(tab.recursive);
        views.push_back(viewModes_.value(tab.id, 0));
    }
    settings.setValue("tabs", paths);
    settings.setValue("tabOptions/recursive", recursiveModes);
    settings.setValue("tabOptions/views", views);
}
void MainWindow::syncWatches() {
    if (closing_)
        return;
    QSet<QString> wanted;
    for (const auto &tab : session_->tabs()) {
        if (tab.location.kind == Location::Kind::Directory)
            wanted.insert(tab.location.path);
        for (const auto &path : tab.watchedDirectories) {
            if (wanted.size() >= 512) break;
            wanted.insert(path);
        }
    }
    // Directory events cover create/delete/rename. Also monitor displayed files
    // for in-place writes, with a bound on native watcher resources.
    if (const auto *tab = session_->tab(currentTabId()))
        for (const auto &entry : tab->entries) {
            if (wanted.size() >= 512)
                break;
            if (!entry.directory && !entry.link)
                wanted.insert(entry.path);
        }
    QSet<QString> existing;
    for (const auto &path : fileWatcher_->directories()) existing.insert(path);
    for (const auto &path : fileWatcher_->files()) existing.insert(path);
    const auto remove = existing - wanted;
    const auto add = wanted - existing;
    if (!remove.isEmpty()) fileWatcher_->removePaths(remove.values());
    if (!add.isEmpty()) fileWatcher_->addPaths(add.values());
}
void MainWindow::requestPreview(const Entry *entry) {
    ++previewGeneration_;
    previewText_->hide();
    previewText_->clear();
    previewPath_ = entry && !entry->directory && !entry->link ? entry->path : QString();
    previewTimer_.stop();
    if (!previewPath_.isEmpty())
        previewTimer_.start();
}
void MainWindow::loadPreview() {
    if (closing_ || previewPath_.isEmpty())
        return;
    if (previewBusy_) {
        previewTimer_.start();
        return;
    }
    struct Preview { QImage image; QString text; };
    const auto path = previewPath_;
    const auto generation = previewGeneration_;
    previewBusy_ = true;
    auto *watcher = new QFutureWatcher<Preview>(this);
    connect(watcher, &QFutureWatcher<Preview>::finished, this, [this, watcher, generation] {
        const auto preview = watcher->result();
        watcher->deleteLater();
        previewBusy_ = false;
        if (closing_ || generation != previewGeneration_)
            return;
        if (!preview.image.isNull())
            detailImage_->setPixmap(QPixmap::fromImage(preview.image));
        if (!preview.text.isEmpty()) {
            previewText_->setPlainText(preview.text);
            previewText_->show();
        }
    });
    watcher->setFuture(QtConcurrent::run(&previewPool_, [path]() -> Preview {
        const QFileInfo info(path);
        if (!info.isFile() || info.isSymLink() || info.size() > 32 * 1024 * 1024)
            return {};
        QImageReader reader(path);
        const auto size = reader.size();
        if (size.isValid() && qint64(size.width()) * size.height() <= 32000000) {
            reader.setAutoTransform(true);
            reader.setScaledSize(size.scaled(250, 180, Qt::KeepAspectRatio));
            const auto image = reader.read();
            if (!image.isNull())
                return {image, {}};
        }
        const QStringList extensions{"txt", "md", "log", "json", "xml", "csv", "ini", "yaml",
                                     "yml", "cpp", "hpp", "h", "c", "py", "js", "ts", "css",
                                     "html", "sh", "ps1", "cmake"};
        if (!extensions.contains(info.suffix().toLower()) && info.fileName() != "CMakeLists.txt")
            return {};
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        const auto bytes = file.read(32768);
        if (bytes.contains('\0'))
            return {};
        return {{}, QString::fromUtf8(bytes) + (file.atEnd() ? QString() : QStringLiteral("\n… (preview truncated)"))};
    }));
}
QJsonObject MainWindow::snapshot() const {
    QJsonObject result;
    const auto *state = session_->tab(currentTabId());
    if (!state)
        return result;
    result["path"] = state->location.path;
    result["kind"] = state->location.kind == Location::Kind::Drives ? "drives" :
                     state->location.kind == Location::Kind::Shell ? "shell" : "directory";
    result["recursive_search"] = state->recursive;
    result["search_warnings"] = QJsonArray::fromStringList(state->searchWarnings);
    result["loading"] = state->loading;
    result["count"] = state->visible.size();
    result["history_cursor"] = state->history.cursor();
    result["tab_count"] = session_->tabs().size();
    result["view_mode"] = viewMode_;
    const auto operation = session_->operationState();
    result["operation_running"] = operation.running;
    if (operation.report) {
        QJsonArray items;
        for (const auto &item : operation.report->items) {
            QJsonObject value{{"source", item.sourcePath},
                              {"destination", item.destinationPath},
                              {"status", static_cast<int>(item.status)}};
            if (item.error)
                value["error"] = item.error->message;
            items.append(value);
        }
        result["operation_report"] = items;
        result["cleanup_pending"] = QJsonArray::fromStringList(operation.report->cleanupPending);
    }
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
