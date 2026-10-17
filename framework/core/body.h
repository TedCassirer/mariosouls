#pragma once
#include "../sdk/character_api.h"
#include <algorithm>
#include <cmath>

namespace SCBody {
inline bool valid(const SC_Body &b) {
    const float values[] = {b.modelScale,
                            b.radiusCm,
                            b.heightCm,
                            b.hurtRadiusCm,
                            b.hurtHeightCm,
                            b.fallDamageScale,
                            b.fallDamageMaxHPFraction};
    for (float v : values)
        if (!std::isfinite(v))
            return false;
    if (b.modelScale < .25f || b.modelScale > 2 || b.fallDamageScale < 0 || b.fallDamageScale > 1 ||
        b.fallDamageMaxHPFraction < 0 || b.fallDamageMaxHPFraction > 1 || b.nonlethalFalls > 1)
        return false;
    if (b.radiusCm == 0 && b.heightCm == 0 && b.hurtRadiusCm == 0 && b.hurtHeightCm == 0)
        return true;
    return b.radiusCm >= 10 && b.radiusCm <= 40 && b.heightCm >= b.radiusCm * 2 &&
           b.heightCm <= 150 && b.hurtRadiusCm >= 10 && b.hurtRadiusCm <= 45 &&
           b.hurtHeightCm >= b.hurtRadiusCm * 2 && b.hurtHeightCm <= 160;
}

inline int landingDamage(int base, int hp, int maxHP, const SC_Body &b) {
    if (base <= 0 || hp <= 0 || maxHP <= 0)
        return 0;
    int scaled = int(std::ceil(base * b.fallDamageScale));
    int cap = int(std::floor(maxHP * b.fallDamageMaxHPFraction));
    return std::max(0, std::min({scaled, cap, b.nonlethalFalls ? hp - 1 : hp}));
}

struct Capsule {
    float radius, top, bottom;
};

inline Capsule capsule(float radiusCm, float heightCm) {
    float r = radiusCm * .01f;
    return {r, heightCm * .01f - r, r};
}

inline float modelCoordinate(float vertex, float feet, float scale) {
    return feet + (vertex - feet) * scale;
}
} // namespace SCBody
