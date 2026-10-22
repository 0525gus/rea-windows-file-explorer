#pragma once
#include "types.h"
#include <optional>

namespace explorer {
class History {
  public:
    void visit(const Location &location);
    void commitCursor(int cursor);
    std::optional<Location> at(int cursor) const;
    int cursor() const {
        return cursor_;
    }
    bool canBack() const {
        return cursor_ > 0;
    }
    bool canForward() const {
        return cursor_ + 1 < entries_.size();
    }
    int size() const {
        return int(entries_.size());
    }

  private:
    QVector<Location> entries_;
    int cursor_ = -1;
};
Location parentOf(const Location &location);
} // namespace explorer
