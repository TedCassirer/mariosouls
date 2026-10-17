#pragma once
#include <cmath>
#include "../sdk/character_api.h"

namespace SCProjectile {
inline bool range(float x, float a, float b) {
    return std::isfinite(x) && x >= a && x <= b;
}

inline bool valid(const SC_ProjectileProfile *p) {
    return p && p->size == sizeof(*p) && p->version == SC_PROJECTILE_PROFILE_VERSION &&
           p->kind == SC_PROJECTILE_FIREBALL && range(p->castSpeed, 1, 5) &&
           range(p->speed, 100, 2000) && range(p->gravity, 100, 5000) &&
           range(p->bounceSpeed, 100, 1000) && range(p->lifetime, .5f, 5) &&
           range(p->spawnHeight, 30, 150);
}

inline bool floorBounce(float ny, float normSquared, float vy, unsigned bounces) {
    return range(ny, .65f, 1.01f) && range(normSquared, .9f, 1.1f) && range(vy, -100, -.01f) &&
           bounces < 12;
}

inline bool sameFloorContact(float ny, float normSquared, float heightDifference,
                             float secondsSinceBounce, float vy) {
    return range(ny, .1f, 1.01f) && range(normSquared, .9f, 1.1f) &&
           range(heightDifference, -.12f, .12f) && range(secondsSinceBounce, 0, .075f) && vy > 0;
}

// Compare-and-restore for the temporary native animation-rate lease.
struct Rate {
    float original = 1, last = 1;
    bool held = false, conflict = false;

    bool acquire(float current, float scale, float &value) {
        if (!range(current, .1f, 5) || !range(scale, 1, 5))
            return false;
        if (!held) {
            original = last = current;
            held = true;
        }
        if (conflict || current != last) {
            conflict = true;
            return false;
        }
        value = original * scale;
        return range(value, .1f, 10);
    }

    bool restore(float current, float &value) {
        bool ok = held && !conflict && current == last;
        value = original;
        held = false;
        return ok;
    }
};
} // namespace SCProjectile
