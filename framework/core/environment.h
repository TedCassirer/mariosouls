#pragma once
#include "../sdk/environment_api.h"

namespace SCEnvironment {
// Animation retrigger cooldown only: never changes damage or protection.
struct Cooldown {
    bool started = false;
    uint32_t last = 0;

    bool ready(uint32_t now, uint32_t duration) const {
        return !started || uint32_t(now - last) >= duration;
    }

    void commit(uint32_t now) {
        started = true;
        last = now;
    }
};

inline bool valid(const SC_EnvironmentAPI *api) {
    return api && api->size == sizeof(*api) && api->version == SC_ENVIRONMENT_VERSION &&
           api->observe;
}

// One launch per grounded contact, with a short debounce for noisy contact.
// Airborne movement and leaving the floor rearm it; holding contact cannot
// restart the animation every frame. Protection/native control always win.
struct ContactReaction {
    bool latched = false, started = false;
    uint32_t last = 0;

    void reset() {
        *this = {};
    }

    bool begin(const SC_Environment &e) {
        const bool ready =
            (e.flags & (SC_ENV_CONTACT | SC_ENV_GROUNDED | SC_ENV_CONTROL_ALLOWED)) ==
            (SC_ENV_CONTACT | SC_ENV_GROUNDED | SC_ENV_CONTROL_ALLOWED);
        if (!ready || (e.flags & SC_ENV_PROTECTED)) {
            latched = false;
            return false;
        }
        if (latched || (started && uint32_t(e.tickMs - last) < 300))
            return false;
        latched = started = true;
        last = e.tickMs;
        return true;
    }
};
} // namespace SCEnvironment
