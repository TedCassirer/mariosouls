#pragma once
#include <cstdint>

namespace SCNativeAction {
// A focus/device/actor boundary requires release, never a synthetic press.
class Buttons {
    uintptr_t actor = 0;
    int device = -2;
    bool focused = false, attackHeld = true, parryHeld = true, consumed = false;

public:
    struct Edges {
        bool attack = false, parry = false;
    };

    Edges sample(bool attack, bool parry, bool focus, uintptr_t owner, int slot) {
        bool ready = focused && focus && actor == owner && device == slot;
        Edges e = {ready && attack && !attackHeld, ready && parry && !parryHeld};
        actor = owner;
        device = slot;
        focused = focus;
        attackHeld = attack;
        parryHeld = parry;
        if (!attack)
            consumed = false;
        if (!focus)
            consumed = true;
        return e;
    }

    void consume() {
        consumed = true;
    }

    bool suppressAttack() const {
        return consumed;
    }
};

// Completion requires observed native recovery. A timer only covers requests
// which the native evaluator declined; it never cuts short an active animation.
class Completion {
    uint32_t began = 0, quietAt = 0;
    bool seen = false, quiet = false;

public:
    void begin(uint32_t now) {
        began = now;
        seen = quiet = false;
    }

    bool update(bool busy, bool idle, bool focus, uint32_t now) {
        seen |= busy;
        if (busy || !idle || !focus) {
            quiet = false;
            return false;
        }
        if (!quiet) {
            quiet = true;
            quietAt = now;
        }
        return uint32_t(now - began) >= (seen ? 150u : 600u) && uint32_t(now - quietAt) >= 100;
    }
};
} // namespace SCNativeAction
