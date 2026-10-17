#pragma once
#include "../sdk/character_api.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace SCFeedback {
inline bool pending(uint32_t until, uint32_t now) {
    return until && int32_t(until - now) > 0;
}

struct Envelope {
    unsigned stopMs, rumbleMs, flashMs;
    float strength;
};

inline Envelope envelope(float multiplier) {
    float w = std::clamp((multiplier - .65f), 0.f, 1.f);
    return {unsigned(34 + 33 * w), unsigned(80 + 55 * w), unsigned(180 + 50 * w), w};
}

// The native call returns a status, not damage. Track a short-lived receipt and
// require an observed HP decrease. A missing/recycled target is not a hit.
struct Receipt {
    uintptr_t target = 0;
    int id = 0, serial = 0, move = 0, before = 0;
    uint32_t began = 0;
    bool object = false;
    float x = 0, y = 0, z = 0, multiplier = 1;

    int sample(int identity, int hp, bool valid, uint32_t now) const {
        if (!valid || identity != id || uint32_t(now - began) > 180)
            return -1;
        if (hp >= 0 && hp < before)
            return 1;
        return hp > before ? -1 : 0;
    }
};

struct Group {
    int serials[16] = {};
    unsigned cursor = 0;
    bool used[16] = {};

    bool first(int serial) {
        for (int i = 0; i < 16; i++)
            if (used[i] && serials[i] == serial)
                return false;
        serials[cursor] = serial;
        used[cursor] = true;
        cursor = (cursor + 1) % 16;
        return true;
    }
};

struct Pause {
    uint32_t until = 0, bufferUntil = 0;
    uint8_t jump = 0, attack = 0, crouch = 0;

    void start(uint32_t now, unsigned ms) {
        until = now + std::min(ms, 80u);
    }

    bool filter(SC_Input &input, uint32_t now) {
        if (pending(until, now)) {
            jump |= input.jump;
            attack |= input.attack;
            crouch |= input.crouch;
            bufferUntil = now + 150;
            return true;
        }
        if (pending(bufferUntil, now)) {
            input.jump |= jump;
            input.attack |= attack;
            input.crouch |= crouch;
        }
        clear();
        return false;
    }

    void clear() {
        until = bufferUntil = 0;
        jump = attack = crouch = 0;
    }
};

inline uint16_t motor(uint16_t native, uint16_t impact, bool active) {
    return active ? std::max(native, impact) : native;
}

// Original procedural percussion: a falling low thump and a short noise crack.
// Stereo signed PCM; bounded amplitude and smooth tail, no external assets.
inline void sound(int16_t *pcm, unsigned frames, unsigned rate, float strength, uint32_t seed,
                  float volume) {
    strength = std::clamp(strength, 0.f, 1.f);
    volume = std::clamp(volume, 0.f, 1.f);
    float phase = 0, filtered = 0;
    seed |= 1;
    for (unsigned i = 0; i < frames; i++) {
        float t = float(i) / rate;
        seed = 1664525u * seed + 1013904223u;
        float noise = float((seed >> 8) & 0xffff) / 32767.5f - 1;
        filtered = .6f * filtered + .4f * noise;
        phase += 6.2831853f * (65 + 75 * (1 - strength) * expf(-t * 45)) / rate;
        float attack = std::min(1.f, t * 1800), tail = std::max(0.f, 1 - float(i) / frames);
        float v = attack * tail *
                  (.62f * sinf(phase) * expf(-t * (38 - 12 * strength)) +
                   .44f * (noise - filtered) * expf(-t * 100));
        auto s = int16_t(std::clamp(v * volume, -.95f, .95f) * 32767);
        pcm[i * 2] = pcm[i * 2 + 1] = s;
    }
}
} // namespace SCFeedback
