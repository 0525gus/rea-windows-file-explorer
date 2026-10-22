#pragma once
#include "types.h"
#include <functional>
#include <optional>
namespace explorer {
enum class OperationKind { Copy, Move, Trash, Remove, Rename, NewFolder, Cleanup };
enum class CollisionPolicy { Ask, Skip, Replace };
struct OperationRequest {
    OperationKind kind = OperationKind::Copy;
    QStringList sources;
    QString destination;
    QString name;
    CollisionPolicy collision = CollisionPolicy::Ask;
    bool confirmedDestructive = false;
};
struct OperationProgress {
    QString phase;
    QString path;
    qint64 bytes = 0;
    qint64 totalBytes = 0;
    int items = 0;
    int totalItems = 0;
};
struct ItemOutcome {
    QString source;
    QString destination;
    bool succeeded = false;
    bool skipped = false;
    std::optional<Error> error;
    QStringList retainedPaths;
    QStringList cleanupPaths;
};
struct OperationReport {
    QVector<ItemOutcome> items;
    bool cancelled = false;
    bool success() const {
        for (const auto &item : items)
            if (item.error)
                return false;
        return !cancelled;
    }
};
using ProgressCallback = std::function<void(const OperationProgress &)>;
using OperationResult = Result<OperationReport>;
OperationResult performFilesystemOperation(const OperationRequest &, const ProgressCallback &,
                                           const CancelToken &);
Result<Success> retryFilesystemCleanup(const QString &path);
} // namespace explorer
