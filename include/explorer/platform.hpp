#pragma once
#include "explorer/types.hpp"
#include <memory>
#include <QImage>
#include <QPoint>
#include <stop_token>
namespace explorer {
class IPlatform {
  public:
    virtual ~IPlatform() = default;
    virtual Result<Location> parseLocation(const QString &input) const = 0;
    virtual Result<std::optional<Location>> parent(const Location &location) const = 0;
    virtual Result<Listing> list(const Location &location, std::stop_token stop = {},
                                 ProgressSink progress = {}) const = 0;
    virtual int compareNames(const QString &a, const QString &b) const = 0;
    virtual Result<Unit> open(const Entry &entry) const = 0;
    virtual Capabilities capabilities() const = 0;
    virtual Result<Entry> inspectEntry(const QString &path) const {
        return Error{ErrorCode::Unsupported, QStringLiteral("inspect item"), path,
                     QStringLiteral("External file inputs are unavailable on this platform")};
    }
    virtual OperationReport execute(const OperationRequest &request, std::stop_token = {},
                                    OperationProgressSink = {}) const {
        OperationReport report;
        report.kind = request.kind;
        report.items.push_back({{},
                                {},
                                ItemStatus::Failed,
                                Error{ErrorCode::Unsupported,
                                      QStringLiteral("file operation"),
                                      {},
                                      QStringLiteral("File operations are unavailable on this platform")},
                                {}});
        return report;
    }
    virtual OperationHistory operationHistory() const { return {}; }
    virtual OperationReport replayOperation(bool redo, std::stop_token = {}, OperationProgressSink = {}) const {
        OperationReport report;
        report.kind = redo ? OperationKind::Redo : OperationKind::Undo;
        report.items.push_back({{}, {}, ItemStatus::Failed,
            Error{ErrorCode::Unsupported, "operation history", {}, "No reversible operation is available"}, {}});
        return report;
    }
    virtual Result<Unit> showContextMenu(const Location &, const QVector<Entry> &, quintptr, QPoint) const {
        return Error{ErrorCode::Unsupported, "Shell menu", {}, "Native Shell menus are unavailable"};
    }
    virtual QImage thumbnail(const Entry &, int) const { return {}; }
    virtual Result<Unit> retryCleanup(const QString &) const {
        return Error{ErrorCode::Unsupported,
                     QStringLiteral("cleanup"),
                     {},
                     QStringLiteral("No owned cleanup retry is available")};
    }
};
std::shared_ptr<IPlatform> makePlatform();
} // namespace explorer
