#pragma once
#include "explorer/types.hpp"
#include <memory>
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
    virtual Result<Unit> retryCleanup(const QString &) const {
        return Error{ErrorCode::Unsupported,
                     QStringLiteral("cleanup"),
                     {},
                     QStringLiteral("No owned cleanup retry is available")};
    }
};
std::shared_ptr<IPlatform> makePlatform();
} // namespace explorer
