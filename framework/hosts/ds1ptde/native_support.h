#pragma once
#include "../../core/motion.h"
#include "memory.h"
#include <cstring>

namespace PTDESupport {
static bool grounded(uintptr_t physics, bool &out) {
    uint8_t supported = 255;
    out = false;
    if (!physics || !SCNativeMemory::get(physics + 0x32, supported) || supported > 1)
        return false;
    out = supported != 0;
    return true;
}

// EBDEC0 calls Havok checkSupport, returns whether its support enum is nonzero;
// EBE3F9 stores that result at ChrPhysics+32. EBE0A6 stores the selected contact
// normal at +170. EBD260 assigns +1C8 only for map/object owners (kinds 0/1).
// This is read-only evidence; native physics supplies all platform transport.
static bool available() {
    const uint8_t grounded[] = {0x88, 0x46, 0x32, 0xc7, 0x84, 0x24, 0x80, 0x03, 0, 0};
    const uint8_t normal[] = {0xf3, 0x0f, 0x7e, 0x44, 0x24, 0x40, 0x66,
                              0x0f, 0xd6, 0x87, 0x70, 0x01, 0,    0};
    const uint8_t owner[] = {0x89, 0x8e, 0xc8, 0x01, 0, 0, 0x85, 0xc9};
    uint8_t a[sizeof(grounded)]{}, b[sizeof(normal)]{}, c[sizeof(owner)]{};
    return SCNativeMemory::get(uintptr_t(0xebe3f9), a) && !memcmp(a, grounded, sizeof(a)) &&
           SCNativeMemory::get(uintptr_t(0xebe0a6), b) && !memcmp(b, normal, sizeof(b)) &&
           SCNativeMemory::get(uintptr_t(0xebd27f), c) && !memcmp(c, owner, sizeof(c));
}

static bool read(uintptr_t physics, SCMotion::GroundContact &out) {
    uint8_t supported = 255;
    uint32_t owner = 0;
    SCMotion::Point normal{};
    out = {};
    if (!physics || !SCNativeMemory::get(physics + 0x32, supported) || supported > 1)
        return false;
    if (!supported)
        return true; // Stale owner/normal fields cannot keep a departed floor alive.
    if (!SCNativeMemory::get(physics + 0x1c8, owner) || !owner ||
        !SCNativeMemory::get(physics + 0x170, normal) || !SCMotion::finite(normal))
        return false;
    float nn = SCMotion::squared(normal);
    if (nn < .8f || nn > 1.2f || normal.y <= .7f)
        return false;
    out = {owner, normal, true};
    return true;
}
} // namespace PTDESupport
