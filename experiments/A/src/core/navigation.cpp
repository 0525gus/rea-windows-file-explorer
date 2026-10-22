#include "explorer/navigation.h"

namespace explorer {
void History::visit(const Location &location) {
    if (cursor_ >= 0 && entries_[cursor_] == location)
        return;
    entries_.resize(cursor_ + 1);
    entries_.push_back(location);
    cursor_ = int(entries_.size()) - 1;
}
void History::commitCursor(int cursor) {
    if (cursor >= 0 && cursor < entries_.size())
        cursor_ = cursor;
}
std::optional<Location> History::at(int cursor) const {
    if (cursor < 0 || cursor >= entries_.size())
        return std::nullopt;
    return entries_[cursor];
}
Location parentOf(const Location &location) {
    if (location.computer)
        return {};
    const auto parent = QDir::cleanPath(location.path + QStringLiteral("/.."));
    if (QDir(location.path).isRoot() || parent == location.path)
        return {};
    return Location::folder(parent);
}
} // namespace explorer
