#pragma once
#include <cmath>
#include <cstdint>
#include "../sdk/character_api.h"

namespace SCImpact {
inline bool valid(const SC_ImpactProfile *p, unsigned count) {
    if (!p || p->size != sizeof(*p) || p->version != SC_IMPACT_PROFILE_VERSION ||
        p->attackCount != count || count > SC_MAX_ATTACKS || p->cooldownMs < 1000 ||
        p->cooldownMs > 5000 || !std::isfinite(p->resistantPoise) || p->resistantPoise < 20 ||
        p->resistantPoise > 200)
        return false;
    for (float d : p->knockbackCm)
        if (!std::isfinite(d) || d < 0 || d > 200)
            return false;
    return true;
}

// Only request a push at a possible native poise break. The native engine still
// decides guard, invulnerability, reaction remapping and collision response.
inline float distance(float requested, float current, float maximum, float damage, float resistant,
                      float radius, float height) {
    if (!std::isfinite(current) || !std::isfinite(maximum) || !std::isfinite(radius) ||
        !std::isfinite(height) || current < 0 || maximum < 0 || current > maximum + .01f ||
        maximum >= resistant || current >= damage || damage <= 0 || radius <= 0 || radius > .8f ||
        height <= 0 || height > 2.8f)
        return 0;
    return requested * .01f * (1 - maximum / resistant);
}

struct Cooldown {
    uintptr_t actor = 0;
    uint32_t last = 0;
    bool active = false;

    bool ready(uintptr_t who, uint32_t now, uint32_t interval) const {
        return !active || actor != who || uint32_t(now - last) >= interval;
    }

    void commit(uintptr_t who, uint32_t now) {
        actor = who;
        last = now;
        active = true;
    }
};
} // namespace SCImpact
