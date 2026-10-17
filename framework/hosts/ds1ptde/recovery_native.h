#pragma once
#include "memory.h"
#include "reaction_types.h"
#include <cstring>

namespace PTDERecovery {
// Owned c0000 passive ESD: only ordinary incoming damage states. Parry,
// guard break, deflection, death, throws and scripted actions are excluded.
inline PTDEReaction::Kind kind(int state) {
    return state >= 41 && state <= 46   ? PTDEReaction::Kind::Flinch
           : state >= 47 && state <= 53 ? PTDEReaction::Kind::Knockdown
                                        : PTDEReaction::Kind::Other;
}

// A knockdown can leave states 45-53 for a looping fall. The fall must reach
// native ground contact; only its subsequent landing clip may finish early.
inline bool airborneDamage(int state) {
    return state == 29 || state == 30;
}

inline bool airborneLanding(int state) {
    return state == 35 || state == 36;
}

inline bool available() {
    const uint8_t predicate[] = {0x51, 0x53, 0x55, 0x56, 0x8b, 0x70, 0x10,
                                 0x8b, 0x6e, 0x18, 0x57, 0x8d, 0x78, 0x60};
    // Chr test 0 (IsGeneralAnimEnd): EDI=ActionCtrl, EAX register ABI.
    const uint8_t dispatch[] = {0x8b, 0xc7, 0xe8, 0xb2, 0x4f, 0, 0};
    // EzState normal transition obtains and replaces current state at +10.
    const uint8_t transition[] = {0x8b, 0xbc, 0x24, 0x68, 0x02, 0, 0, 0x89, 0x7e, 0x10};
    uint8_t a[sizeof(predicate)]{}, b[sizeof(dispatch)]{}, c[sizeof(transition)]{};
    return SCNativeMemory::get(0xe2c340, a) && !memcmp(a, predicate, sizeof(a)) &&
           SCNativeMemory::get(0xe27387, b) && !memcmp(b, dispatch, sizeof(b)) &&
           SCNativeMemory::get(0x73f2ab, c) && !memcmp(c, transition, sizeof(c));
}

struct State {
    uint32_t movement = 0, action = 0, machine = 0, entry = 0;
    int id = -1;
};

inline bool inspect(uintptr_t actor, State &out) {
    out = {};
    uint32_t owner = 0, vtable = 0;
    return actor && SCNativeMemory::get(actor + 0x28, out.movement) && out.movement &&
           SCNativeMemory::get(out.movement + 0x2c, out.action) && out.action &&
           SCNativeMemory::get(out.action + 0x10, owner) && owner == out.movement &&
           SCNativeMemory::get(out.action + 0x20, out.machine) && out.machine &&
           SCNativeMemory::get(out.machine, vtable) && vtable == 0x11045cc &&
           SCNativeMemory::get(out.machine + 4, owner) && owner == out.action &&
           SCNativeMemory::get(out.machine + 0x10, out.entry) && out.entry &&
           SCNativeMemory::get(out.entry, out.id);
}

struct Request {
    uintptr_t actor = 0;
    State state;
    int hp = 0, map = -1;
    uint32_t tick = 0;
    bool landedAirborneDamage = false;
};

inline bool matches(const Request &r, uintptr_t actor, uintptr_t action, int hp, int map,
                    uint32_t now, const State &state, bool permitted) {
    return permitted && actor && actor == r.actor && action == r.state.action && hp > 0 &&
           hp == r.hp && map == r.map && uint32_t(now - r.tick) <= 100 &&
           state.movement == r.state.movement && state.action == r.state.action &&
           state.machine == r.state.machine && state.entry == r.state.entry &&
           state.id == r.state.id &&
           (kind(state.id) != PTDEReaction::Kind::Other ||
            (r.landedAirborneDamage && airborneLanding(state.id)));
}
} // namespace PTDERecovery
