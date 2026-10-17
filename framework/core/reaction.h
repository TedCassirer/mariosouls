#pragma once
#include "../sdk/character_api.h"
#include <cmath>

namespace SCReaction {
inline bool validRecovery(const SC_RecoveryProfile *p) {
    return p && p->size == sizeof(*p) && p->version == SC_RECOVERY_PROFILE_VERSION &&
           p->flinchMs >= 100 && p->flinchMs <= 1000 && p->knockdownMs >= 200 &&
           p->knockdownMs <= 2000 && std::isfinite(p->knockdownSpeed) && p->knockdownSpeed >= 1 &&
           p->knockdownSpeed <= 5;
}

// Presentation runs once per hit. Pauses do not consume its animation, and
// completion may request a verified ordinary native reaction's normal exit;
// it is never authority to clear native control flags or interrupt scripted states.
class Presentation {
    float elapsed = 0;

public:
    void reset() {
        elapsed = 0;
    }

    void advance(float seconds, bool running) {
        if (running && std::isfinite(seconds) && seconds > 0 && seconds <= .1f)
            elapsed += seconds;
    }

    float phase(uint32_t durationMs) const {
        return durationMs ? std::fmin(1.f, elapsed * 1000 / durationMs) : 1.f;
    }
};

inline bool valid(const SC_ReactionProfile *p) {
    return p && p->size == sizeof(*p) && p->version == SC_REACTION_PROFILE_VERSION &&
           p->returnQuietMs >= 16 && p->returnQuietMs <= 100 && std::isfinite(p->bonusPoise) &&
           p->bonusPoise >= 0 && p->bonusPoise <= 60 && std::isfinite(p->lightReactionSpeed) &&
           p->lightReactionSpeed >= 1 && p->lightReactionSpeed <= 1.5f;
}

inline float poise(float base, float bonus) {
    return std::isfinite(base) && base >= 0 && base <= 1000 && std::isfinite(bonus) && bonus >= 0 &&
                   bonus <= 60
               ? base + bonus
               : base;
}

// HP loss is not proof of a poise break. Native passive ownership decides the
// reaction; ordinary navigation must never add a guest knockback on top of it.
inline bool guestHurt(bool nativeReactions, int activeAllowed, bool launcher) {
    return !nativeReactions || activeAllowed < 0 || launcher;
}
} // namespace SCReaction
