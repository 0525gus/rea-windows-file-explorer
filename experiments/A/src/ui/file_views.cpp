#include "explorer/file_views.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QUrl>
#include <optional>
namespace explorer {
Result<QStringList> localPaths(const QMimeData *mime) {
    if (!mime || !mime->hasUrls())
        return Error{ErrorCode::Unsupported, "Drop", {}, "Only local file URLs are supported.", 0};
    QStringList paths;
    for (const auto &url : mime->urls()) {
        if (!url.isLocalFile())
            return Error{ErrorCode::Unsupported, "Drop", url.toString(),
                         "Remote URLs are not file-operation inputs.", 0};
        const auto path = url.toLocalFile();
        if (path.isEmpty() || !QDir::isAbsolutePath(path))
            return Error{ErrorCode::InvalidName, "Drop", path, "A local absolute file path is required.", 0};
        if (!paths.contains(path))
            paths.push_back(path);
    }
    if (paths.isEmpty())
        return Error{ErrorCode::InvalidName, "Drop", {}, "No local files were supplied.", 0};
    return paths;
}
FileViewInput::FileViewInput(QAbstractItemView *view, FileProxy *proxy, QObject *parent)
    : QObject(parent), view_(view), proxy_(proxy) {
    view_->setDragEnabled(true);
    view_->setAcceptDrops(true);
    view_->viewport()->setAcceptDrops(true);
    view_->setDropIndicatorShown(true);
    view_->setDragDropMode(QAbstractItemView::DragDrop);
    view_->setDefaultDropAction(Qt::CopyAction);
    view_->viewport()->installEventFilter(this);
    view_->installEventFilter(this);
}
bool FileViewInput::eventFilter(QObject *object, QEvent *event) {
    if ((object == view_ || object == view_->viewport()) &&
        (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride)) {
        const auto *key = static_cast<QKeyEvent *>(event);
        std::optional<FileCommand> selected;
        if (key->key() == Qt::Key_Delete && key->modifiers() == Qt::ShiftModifier)
            selected = FileCommand::Remove;
        else if (key->key() == Qt::Key_Delete && key->modifiers() == Qt::NoModifier)
            selected = FileCommand::Trash;
        else if (key->matches(QKeySequence::Copy))
            selected = FileCommand::Copy;
        else if (key->matches(QKeySequence::Cut))
            selected = FileCommand::Cut;
        else if (key->matches(QKeySequence::Paste))
            selected = FileCommand::Paste;
        else if (key->matches(QKeySequence::SelectAll))
            selected = FileCommand::SelectAll;
        else if (key->key() == Qt::Key_F2 && key->modifiers() == Qt::NoModifier)
            selected = FileCommand::Rename;
        else if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) &&
                 key->modifiers() == Qt::NoModifier)
            selected = FileCommand::Open;
        if (selected) {
            event->accept();
            if (event->type() == QEvent::KeyPress)
                emit command(*selected);
            return true;
        }
    }
    if (object != view_->viewport())
        return QObject::eventFilter(object, event);
    if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove ||
        event->type() == QEvent::Drop) {
        auto *drop = static_cast<QDropEvent *>(event);
        const auto parsed = localPaths(drop->mimeData());
        const bool valid = !location_.computer && std::holds_alternative<QStringList>(parsed) &&
                           (drop->possibleActions() & (Qt::CopyAction | Qt::MoveAction));
        if (!valid) {
            drop->ignore();
            if (event->type() == QEvent::Drop)
                emit rejected("Drop requires a local directory and local file URLs.");
            return true;
        }
        auto action = drop->proposedAction();
        if (action != Qt::CopyAction && action != Qt::MoveAction)
            action = Qt::CopyAction;
        drop->setDropAction(action);
        drop->accept();
        if (event->type() == QEvent::Drop) {
            auto destination = location_.path;
            const auto index = view_->indexAt(drop->position().toPoint());
            const auto *model = qobject_cast<FileModel *>(proxy_->sourceModel());
            if (index.isValid())
                if (const auto *entry = model->entry(proxy_->mapToSource(index).row());
                    entry && entry->directory && !entry->link)
                    destination = entry->path;
            emit dropped(std::get<QStringList>(parsed), destination, action == Qt::MoveAction);
        }
        return true;
    }
    return QObject::eventFilter(object, event);
}
} // namespace explorer
