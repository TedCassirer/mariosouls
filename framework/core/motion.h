#pragma once
#include "../sdk/character_api.h"
#include "timing.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace SCMotion {
struct Point {
    float x = 0, y = 0, z = 0;

    Point operator+(Point b) const {
        return {x + b.x, y + b.y, z + b.z};
    }

    Point operator-(Point b) const {
        return {x - b.x, y - b.y, z - b.z};
    }

    Point operator*(float s) const {
        return {x * s, y * s, z * s};
    }
};

inline float squared(Point p) {
    return p.x * p.x + p.y * p.y + p.z * p.z;
}

inline bool finite(Point p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}

struct Ticket {
    uint32_t generation = 0, sequence = 0, created = 0;
    Point from, to;
    float yaw = 0;
};

// Single producer/consumer mailbox. The host adds its own lock. No overwriting,
// replay, late requests, or acknowledgements from a previous ownership epoch.
class Mailbox {
    uint32_t epoch = 0, next = 0;
    bool bound = false, queued = false, submitted = false;
    Ticket ticket{};

public:
    void reset() {
        ++epoch;
        bound = queued = submitted = false;
        ticket = {};
    }

    void bind() {
        reset();
        bound = true;
    }

    bool ready() const {
        return bound && !queued && !submitted;
    }

    bool busy() const {
        return queued || submitted;
    }

    uint32_t generation() const {
        return epoch;
    }

    bool publish(Point from, Point to, float yaw, uint32_t now) {
        if (!ready() || !finite(from) || !finite(to) || !std::isfinite(yaw) ||
            squared(to - from) > 4)
            return false;
        ticket = {epoch, ++next, now, from, to, yaw};
        queued = true;
        return true;
    }

    bool consume(uint32_t now, Ticket &out) {
        if (!bound || !queued)
            return false;
        if (uint32_t(now - ticket.created) > 120) {
            queued = false;
            return false;
        }
        queued = false;
        submitted = true;
        out = ticket;
        return true;
    }

    bool acknowledge(Ticket &out) {
        if (!bound || !submitted)
            return false;
        out = ticket;
        submitted = false;
        return true;
    }

    bool expired(uint32_t now) const {
        return bound && busy() && uint32_t(now - ticket.created) > 250;
    }
};

// Some event-owned platforms are absent from the prepared guest map.
// Positions here are host-world metres. Infer support only when native physics rejects
// successive downward guest requests. Native motion remains authoritative.
class NativeSupport {
    Point previous{};
    bool havePrevious = false, active = false;
    unsigned confirmations = 0;

public:
    void reset() {
        previous = {};
        havePrevious = active = false;
        confirmations = 0;
    }

    bool supported() const {
        return active;
    }

    void observe(const Ticket *ticket, Point host) {
        if (!finite(host)) {
            reset();
            return;
        }
        if (havePrevious && (host.y < previous.y - .05f || squared(host - previous) > 1.f)) {
            active = false;
            confirmations = 0;
        }
        previous = host;
        havePrevious = true;
        if (!ticket)
            return;
        Point wanted = ticket->to - ticket->from;
        if (wanted.y > .025f) {
            active = false;
            confirmations = 0;
            return;
        }
        // A wall or diagonal correction cannot by itself create a floor. The game
        // must have held the actor vertically against a meaningful downward request.
        Point disagreement = host - ticket->to;
        bool held = wanted.y < -.02f && host.y >= ticket->from.y - .015f &&
                    disagreement.y > .035f && std::abs(disagreement.x) < .15f &&
                    std::abs(disagreement.z) < .15f;
        if (held) {
            if (confirmations < 3)
                ++confirmations;
            if (confirmations >= 2)
                active = true;
        } else if (!active)
            confirmations = 0;
    }
};

// Compatibility name; the PTDE caller retains its one-tick-per-update policy.
using Clock = SCTiming::Clock;

class InputLatch {
    SC_Input stored{};
    uint32_t pressedAt = 0;
    bool buffered = false;

public:
    void clear() {
        stored = {};
        buffered = false;
    }

    void sample(const SC_Input &i, uint32_t now) {
        if (buffered && uint32_t(now - pressedAt) > 150)
            clear();
        stored.camLookX = i.camLookX;
        stored.camLookZ = i.camLookZ;
        stored.moveX = i.moveX;
        stored.moveY = i.moveY;
        stored.guard = i.guard; // Held state, never buffered as a press after release.
        if (i.jump || i.attack || i.crouch) {
            if (!buffered)
                pressedAt = now;
            buffered = true;
            stored.jump |= i.jump;
            stored.attack |= i.attack;
            stored.crouch |= i.crouch;
        }
    }

    SC_Input take(uint32_t now) {
        if (buffered && uint32_t(now - pressedAt) > 150) {
            stored.jump = stored.attack = stored.crouch = 0;
        }
        auto out = stored;
        clear();
        return out;
    }
};

// Clip only velocity into a confirmed collision normal. A positional correction
// alone must not manufacture a floor or silently reset an airborne action.
inline Point clip(Point v, Point normal) {
    float n = squared(normal);
    if (n < .8f || n > 1.2f)
        return v;
    float into = v.x * normal.x + v.y * normal.y + v.z * normal.z;
    return into < 0 ? v - normal * into : v;
}
} // namespace SCMotion
