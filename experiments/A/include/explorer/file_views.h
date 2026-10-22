#pragma once
#include "file_model.h"
#include "operations.h"
#include <QAbstractItemView>
#include <QMimeData>
#include <QObject>
namespace explorer {
enum class FileCommand { Open, Copy, Cut, Paste, SelectAll, Rename, Trash, Remove };
Result<QStringList> localPaths(const QMimeData *mime);
class FileViewInput final : public QObject {
    Q_OBJECT
  public:
    FileViewInput(QAbstractItemView *, FileProxy *, QObject *parent = nullptr);
    void setLocation(const Location &location) {
        location_ = location;
    }
  signals:
    void dropped(const QStringList &paths, const QString &destination, bool move);
    void rejected(const QString &message);
    void command(explorer::FileCommand command);

  protected:
    bool eventFilter(QObject *, QEvent *) override;

  private:
    QAbstractItemView *view_;
    FileProxy *proxy_;
    Location location_;
};
} // namespace explorer
