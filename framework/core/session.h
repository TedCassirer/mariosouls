#pragma once
#include <cstdint>

namespace SCSession {
enum class Owner { Native, Character, NativeAction, NativeDeath };

struct Identity {
    uintptr_t actor = 0, physics = 0, movement = 0;
    int map = -1;

    bool valid() const {
        return actor && physics && movement && map >= 0;
    }

    bool operator==(const Identity &b) const {
        return actor == b.actor && physics == b.physics && movement == b.movement && map == b.map;
    }
};

class Session {
    Owner mode = Owner::Native;
    Identity identity{};
    uint32_t epoch = 0;
    bool wanted = false, settling = false;
    uint32_t stableAt = 0, retryAt = 0;

public:
    Owner owner() const {
        return mode;
    }

    bool visible() const {
        return mode != Owner::Native;
    }

    bool character() const {
        return mode == Owner::Character;
    }

    bool action() const {
        return mode == Owner::NativeAction;
    }

    bool death() const {
        return mode == Owner::NativeDeath;
    }

    bool requested() const {
        return wanted;
    }

    uint32_t generation() const {
        return epoch;
    }

    bool matches(Identity next) const {
        return identity.valid() && identity == next;
    }

    bool sameActor(Identity next) const {
        return identity.valid() && next.valid() && identity.actor == next.actor &&
               identity.physics == next.physics && identity.movement == next.movement;
    }

    // A seamless area boundary does not replace the native actor or guest rig.
    // Preserve action/stamina windows and movement; only the host terrain changes.
    bool changeMap(Identity next) {
        if (!character() || !sameActor(next))
            return false;
        identity = next;
        ++epoch;
        return true;
    }

    void request(bool value) {
        wanted = value;
        if (!value)
            settling = false;
    }

    void activate(Identity next) {
        identity = next;
        mode = Owner::Character;
        wanted = true;
        settling = false;
        ++epoch;
    }

    bool handoff() {
        if (!character())
            return false;
        mode = Owner::NativeAction;
        ++epoch;
        return true;
    }

    bool resume() {
        if (!action())
            return false;
        mode = Owner::Character;
        ++epoch;
        return true;
    }

    bool die() {
        if (!visible() || death())
            return false;
        mode = Owner::NativeDeath;
        settling = false;
        ++epoch;
        return true;
    }

    void release() {
        mode = Owner::Native;
        identity = {};
        settling = false;
        ++epoch;
    }

    bool canRetry(Identity next, bool safe, uint32_t now) {
        if (!wanted || visible() || !next.valid() || (retryAt && int32_t(now - retryAt) < 0)) {
            settling = false;
            return false;
        }
        if (!settling || !(identity == next)) {
            identity = next;
            stableAt = now;
            settling = true;
            return false;
        }
        // Settle the loaded identity, not its animation. Walking or a load/menu
        // animation must not restart the timer on every frame. Ownership still waits
        // for a safe native frame, even after the identity has settled.
        return safe && uint32_t(now - stableAt) >= 500;
    }

    void failed(uint32_t now) {
        settling = false;
        retryAt = now + 2000;
    }
};

// Completion stability is time-based, independent of rendering rate.
class QuietGate {
    bool quiet = false;
    uint32_t since = 0;

public:
    void clear() {
        quiet = false;
    }

    bool update(bool ready, uint32_t now) {
        if (!ready) {
            clear();
            return false;
        }
        if (!quiet) {
            quiet = true;
            since = now;
        }
        return uint32_t(now - since) >= 100;
    }
};
} // namespace SCSession
