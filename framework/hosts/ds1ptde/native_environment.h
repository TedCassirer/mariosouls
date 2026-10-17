#pragma once
#include "memory.h"
#include <cstring>

namespace PTDEEnvironment {
struct Effects {
    bool lava = false, protectedLava = false;
};

// Owned PTDE executable, DC02C0: active SpEffect-state query. ChrIns+1E0
// selects its manager; +4 is the list head, node+14 disabled-state flags,
// node+2C param bytes, node+30 next, param+156 stateInfo. Never call the native
// register-convention function or retain transient pointers between frames.
inline bool available() {
    const uint8_t prefix[] = {0xb8, 0x39, 0x8e, 0xe3, 0x38, 0xf7, 0xe6,
                              0x33, 0xc9, 0xd1, 0xea, 0x53, 0x32, 0xdb};
    const uint8_t list[] = {0x8b, 0x47, 0x04, 0x85, 0xc0, 0x74, 0x27};
    const uint8_t state[] = {0x0f, 0xb6, 0x89, 0x56, 0x01, 0, 0};
    uint8_t a[sizeof(prefix)]{}, b[sizeof(list)]{}, c[sizeof(state)]{};
    return SCNativeMemory::get(0xdc02c0, a) && !memcmp(a, prefix, sizeof(a)) &&
           SCNativeMemory::get(0xdc02eb, b) && !memcmp(b, list, sizeof(b)) &&
           SCNativeMemory::get(0xdc0303, c) && !memcmp(c, state, sizeof(c));
}

inline bool read(uintptr_t actor, Effects &out) {
    out = {};
    uint32_t manager = 0, node = 0;
    if (!actor || !SCNativeMemory::get(actor + 0x1e0, manager) || !manager ||
        !SCNativeMemory::get(manager + 4, node))
        return false;
    Effects found;
    uint32_t seen[128]{};
    unsigned count = 0;
    while (node) {
        if (count == 128)
            return false;
        for (unsigned i = 0; i < count; ++i)
            if (seen[i] == node)
                return false;
        seen[count++] = node;
        uint32_t flags = 0, param = 0, next = 0;
        if (!SCNativeMemory::get(node + 0x14, flags) || !SCNativeMemory::get(node + 0x2c, param) ||
            !SCNativeMemory::get(node + 0x30, next))
            return false;
        if (!(flags & 0x800c0003u) && param) {
            uint8_t state = 0;
            if (!SCNativeMemory::get(param + 0x156, state))
                return false;
            // Installed params: lava 4030/4031 state 142; Orange Charred
            // Ring 139 -> effect 2210 state 186. Other fire hits do not qualify.
            found.lava |= state == 142;
            found.protectedLava |= state == 186;
        }
        node = next;
    }
    out = found;
    return true;
}
} // namespace PTDEEnvironment
