#pragma once
#include "../../core/presentation_transform.h"

namespace PTDECutscene {
struct Frame {
    uintptr_t step = 0, scene = 0, cut = 0, part = 0, body = 0, assembly = 0;
    float camera[16]{}, model[16]{}, fov = 0;
    bool active = false, visible = false, usable = false;

    bool same(const Frame &b) const {
        return step == b.step && scene == b.scene && cut == b.cut && part == b.part &&
               body == b.body && assembly == b.assembly;
    }
};

// All offsets are from owned PTDE disassembly. RemoManStep's primary wrapper
// holds RemoStep; EE0F80 sets +B8 only while its scene is playing and publishes
// the camera matrix/FOV. D00550 selects the current cut at scene+3698.
inline Frame observe(bool verified) {
    Frame f;
    if (!verified)
        return f;
    uintptr_t man = read<uint32_t>(0x1378490), wrapper = read<uint32_t>(man + 0x28);
    if (!man || read<uint32_t>(man) != 0x117d580 || !wrapper)
        return f;
    f.step = read<uint32_t>(wrapper + 0x18);
    if (!f.step || read<uint32_t>(f.step) != 0x117d650 || read<uint8_t>(f.step + 0xb8) != 1)
        return {};
    f.active = true;
    f.scene = read<uint32_t>(f.step + 0x4c);
    if (!f.scene || read<uint32_t>(f.scene) != 0x11ac634)
        return f;
    bool cameraReady = get(f.step + 0x70, f.camera) && SCPresentation::rigid(f.camera) &&
                       get(f.step + 0xb0, f.fov) && std::isfinite(f.fov) && f.fov >= .1f &&
                       f.fov <= 2.8f;
    f.cut = read<uint32_t>(f.scene + 0x3698);
    int cuts = read<int>(f.scene + 0x18);
    uintptr_t cutBase = read<uint32_t>(f.scene + 0x1c);
    if (!f.cut || cuts < 1 || cuts > 4096 || !cutBase || f.cut < cutBase ||
        (f.cut - cutBase) % 0x2c || (f.cut - cutBase) / 0x2c >= unsigned(cuts) ||
        read<uint32_t>(f.cut) != 0x11ac248)
        return f;
    int models = read<int>(f.cut + 0x18);
    uintptr_t modelBase = read<uint32_t>(f.cut + 0x1c);
    if (models < 1 || models > 512 || !modelBase)
        return f;
    int totalParts = 0;
    for (int i = 0; i < models; ++i) {
        uintptr_t mdl = modelBase + i * 0x18;
        if (read<uint32_t>(mdl) != 0x11ac260)
            return f;
        int count = read<int>(mdl + 8);
        uintptr_t parts = read<uint32_t>(mdl + 0xc);
        if (count < 0 || count > 4096 - totalParts || (count && !parts))
            return f;
        totalParts += count;
        for (int j = 0; j < count; ++j) {
            uintptr_t p = parts + j * 0x140;
            if (read<uint32_t>(p) != 0x11ac2a8)
                return f;
            uintptr_t data = read<uint32_t>(p + 0xa0);
            // REMO type10 is the player assembly; type2 includes enemies/NPCs.
            if (!data || read<int>(data + 4) != 10)
                continue;
            if (f.part) {
                return f;
            } // refuse ambiguous player copies
            f.part = p;
            f.body = read<uint32_t>(p + 0xac);
            f.assembly = read<uint32_t>(p + 0xb0);
            if (!get(p + 0x30, f.model) || !SCPresentation::rigid(f.model))
                return f;
            uint8_t flags = 0;
            // D013F0/D014F0 use bit2 to hide/show a part within the scene.
            uintptr_t model = f.assembly ? f.assembly : f.body;
            if (!model || !get(model + (f.assembly ? 0x92 : 0x76), flags))
                return f;
            f.visible = !(flags & 2);
        }
    }
    f.usable = f.part != 0 && cameraReady;
    return f;
}

inline bool verify() {
    const unsigned char active[] = {0xc6, 0x87, 0xb8, 0, 0, 0, 1};
    const unsigned char select[] = {0x89, 0x86, 0x98, 0x36, 0, 0};
    const unsigned char player[] = {0x8b, 0x46, 4, 0x83, 0xf8, 2};
    unsigned char a[sizeof(active)]{}, b[sizeof(select)]{}, c[sizeof(player)]{};
    return get(uintptr_t(0xee1086), a) && !memcmp(a, active, sizeof(a)) &&
           get(uintptr_t(0xd004ed), b) && !memcmp(b, select, sizeof(b)) &&
           get(uintptr_t(0xd03266), c) && !memcmp(c, player, sizeof(c));
}
} // namespace PTDECutscene
