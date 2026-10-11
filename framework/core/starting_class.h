#pragma once
#include "../sdk/class_api.h"
#include <cmath>
#include <cstring>

namespace SCClass {
inline bool valid(const SC_ClassProfile *p) {
    return p && p->size == sizeof(*p) && p->archetype == SC_CLASS_FIRECASTER && p->name &&
           p->description && p->name[0] && p->description[0] && std::strlen(p->name) <= 31 &&
           std::strlen(p->description) <= 191 && std::isfinite(p->previewHeightCm) &&
           p->previewHeightCm >= 20 && p->previewHeightCm <= 500 && std::isfinite(p->previewYaw) &&
           std::fabs(p->previewYaw) <= 6.284f;
}
// Class identity is persistent intent, independent of physics/render ownership.
// Explicit manual overrides last for this life/area only; lifecycle boundaries
// reinstate the saved class even when the engine reuses all actor pointers.
enum class Decision { None, Enable, Disable };

struct Selection {
    uintptr_t actor = 0;
    int map = -1;
    bool selected = false, manualOverride = false, alive = false;

    bool required() const {
        return actor && selected && !manualOverride;
    }

    Decision observe(uintptr_t current, bool known, bool matches, int area, bool living) {
        bool previous = selected;
        bool changed = current != actor;
        if (changed) {
            actor = current;
            map = -1;
            selected = false;
            manualOverride = false;
            alive = false;
        }
        if (!current)
            return previous ? Decision::Disable : Decision::None;
        if ((map >= 0 && area >= 0 && map != area) || (!alive && living))
            manualOverride = false;
        if (area >= 0)
            map = area;
        alive = living;
        if (known)
            selected = matches; // unreadable class during streaming retains identity
        if (required())
            return Decision::Enable;
        if (previous && (!selected || changed))
            return Decision::Disable;
        return Decision::None;
    }

    void manual(uintptr_t current) {
        actor = current;
        manualOverride = true;
    }
};
} // namespace SCClass
