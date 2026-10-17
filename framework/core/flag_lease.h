#pragma once
#include <cstdint>

namespace SCFlags {
// Remove only bits introduced by this owner. Preserve pre-existing and unrelated
// engine flags, and relinquish ownership once a temporary request ends.
struct Lease {
    uint32_t introduced = 0;

    uint32_t update(uint32_t current, uint32_t wanted) {
        uint32_t removed = introduced & ~wanted;
        introduced &= wanted;
        current &= ~removed;
        introduced |= wanted & ~current;
        return current | wanted;
    }

    void abandon() {
        introduced = 0;
    }
};
} // namespace SCFlags
