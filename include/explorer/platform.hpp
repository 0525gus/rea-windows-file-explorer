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
};
std::shared_ptr<IPlatform> makePlatform();
} // namespace explorer
