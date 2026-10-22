#pragma once
#include "operations.h"

namespace explorer {
class Platform {
  public:
    virtual ~Platform() = default;
    virtual Capabilities capabilities() const = 0;
    virtual Result<Listing> list(const Location &location, const CancelToken &cancel) const = 0;
    virtual Result<Success> openFile(const QString &path) const = 0;
    virtual OperationResult mutate(const OperationRequest &, const ProgressCallback &,
                                   const CancelToken &) const {
        return Error{ErrorCode::Unsupported,
                     QStringLiteral("File operation"),
                     {},
                     QStringLiteral("This platform does not support file operations."),
                     0};
    }
    virtual Result<Success> retryCleanup(const QString &path) const {
        return Error{ErrorCode::Unsupported, "Cleanup", path, "Cleanup retry is unsupported.", 0};
    }
};
std::shared_ptr<Platform> makeFilesystemPlatform();
} // namespace explorer
