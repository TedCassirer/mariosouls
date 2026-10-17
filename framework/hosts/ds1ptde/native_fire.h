#pragma once
#include "memory.h"
#include <cmath>
#include <cstring>

namespace PTDEFire {
struct Hit {
    uintptr_t actor = 0, movement = 0;
    int before = 0, after = 0;
    bool candidate = false;
    uintptr_t source = 0;
};

inline bool available() {
    const uint8_t prefix[] = {0x55, 0x8b, 0xec, 0x83, 0xe4, 0xf8,
                              0x0f, 0x57, 0xc0, 0x83, 0xec, 0x14};
    const uint8_t commit[] = {0x8a, 0x8b, 0xc5, 0,    0,    0,    0x8b, 0xbb, 0x6c, 0x01, 0,
                              0,    0x80, 0xe1, 0x01, 0xf7, 0xdf, 0xe8, 0x28, 0xe3, 0xff, 0xff};
    uint8_t a[sizeof(prefix)]{}, b[sizeof(commit)]{};
    return SCNativeMemory::get(0xe6bfe0, a) && !memcmp(a, prefix, sizeof(a)) &&
           SCNativeMemory::get(0xe6c052, b) && !memcmp(b, commit, sizeof(b));
}

inline Hit before(uintptr_t actor, uintptr_t attacker, uintptr_t packet, unsigned preview) {
    Hit hit;
    int damage = 0, hp = 0;
    float fire = 0;
    uint32_t movement = 0;
    // Optional source may be a projectile/explosion without a live character
    // owner. Never infer a team from an untyped source. Native fire power and
    // committed HP loss are the evidence; self hits remain excluded.
    if (!preview && actor && actor != attacker && packet && SCNativeMemory::get(packet + 8, fire) &&
        std::isfinite(fire) && fire > 0 && fire < 100000 &&
        SCNativeMemory::get(packet + 0x16c, damage) && damage > 0 && damage < 100000 &&
        SCNativeMemory::get(actor + 0x2d4, hp) && hp > 0 && hp < 100000 &&
        SCNativeMemory::get(actor + 0x28, movement) && movement)
        hit = {actor, movement, hp, 0, true, attacker};
    return hit;
}

inline bool after(Hit &hit) {
    uint32_t movement = 0;
    return hit.candidate && SCNativeMemory::get(hit.actor + 0x28, movement) &&
           movement == hit.movement && SCNativeMemory::get(hit.actor + 0x2d4, hit.after) &&
           hit.after > 0 && hit.after < hit.before;
}
} // namespace PTDEFire
