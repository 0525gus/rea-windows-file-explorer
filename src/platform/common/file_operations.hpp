#pragma once
#include "explorer/platform.hpp"
#include <QMap>
#include <mutex>

namespace explorer::platform_detail {
struct Node {
    QString identity;
    bool directory = false, link = false;
    quint64 size = 0;
    qint64 modified = 0;
};
struct NativeOperations {
    bool windowsPaths = false;
    std::function<Result<Node>(const QString &)> inspect;
    std::function<Result<Node>(const QString &, bool)> createExclusive;
    std::function<Result<bool>(const QString &, const Node &, const QString &, const Node &)> moveNoReplace;
    std::function<Result<Unit>(const QString &, const Node &, bool)> remove;
    std::function<Result<Unit>(const QString &, const Node &, const QString &, const Node &, std::stop_token,
                               const std::function<void(quint64)> &)>
        copyContents;
    std::function<Result<Unit>(const QString &, const Node &)> recycle;
};
class FileOperations {
  public:
    explicit FileOperations(NativeOperations native) : native_(std::move(native)) {}
    OperationReport execute(const OperationRequest &, std::stop_token, OperationProgressSink) const;
    Result<Unit> retryCleanup(const QString &) const;

  private:
    struct OwnedNode {
        QString path;
        Node node;
    };
    using Tree = QVector<OwnedNode>;
    NativeOperations native_;
    mutable std::mutex mutex_;
    mutable QMap<QString, Tree> cleanup_;
    Result<Node> safeInspect(const QString &) const;
    Result<Tree> snapshot(const QString &, std::stop_token) const;
    Result<Unit> verify(const Tree &, bool contents) const;
    Result<Unit> removeTree(Tree &, std::stop_token = {}, bool contents = false) const;
};
} // namespace explorer::platform_detail
