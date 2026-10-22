#pragma once
#include "explorer/platform.hpp"
#include "../common/file_operations.hpp"

namespace explorer::windows_shell {
Result<Location> parse(const QString &);
Result<std::optional<Location>> parent(const Location &);
Result<Listing> list(const Location &, std::stop_token, ProgressSink,
                     const std::function<void(Entry &)> &);
Result<Unit> open(const Entry &);
Result<Unit> menu(const Location &, const QVector<Entry> &, quintptr, QPoint);
QImage thumbnail(const Entry &, int);
Result<platform_detail::TrashReceipt> recycle(const QString &, const QString &identity);
Result<QString> restore(const platform_detail::TrashReceipt &, const QString &);
}
