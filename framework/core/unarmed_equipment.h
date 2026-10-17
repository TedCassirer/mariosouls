#pragma once
#include "../sdk/character_api.h"
#include <cstring>
#include <cstdint>

namespace SCUnarmed {
template <size_t N> inline bool text(const char (&s)[N]) {
    return s[0] && std::memchr(s, 0, N);
}

inline bool valid(const SC_UnarmedEquipmentProfile *p) {
    return p && p->size == sizeof(*p) && p->version == SC_UNARMED_EQUIPMENT_PROFILE_VERSION &&
           p->slot == SC_UNARMED_STANDARD_FIST && p->maximumUpgrade == 15 && text(p->name) &&
           text(p->summary) && text(p->description);
}

// A switch during an attack must return to a neutral action before rearming.
struct Gate {
    int item = -2;
    bool blocked = false;

    void reset() {
        item = -2;
        blocked = false;
    }

    bool observe(bool equipped, int identity, int attack) {
        if (identity != item) {
            item = identity;
            blocked = attack >= 0;
        }
        if (attack < 0)
            blocked = false;
        return equipped && !blocked;
    }
};
} // namespace SCUnarmed
