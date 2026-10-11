#pragma once
#include "../../core/starting_class.h"
static SC_ClassProfile classProfile{};
static bool classEnabled = false, classHooksReady = false;
static wchar_t className[64] = {}, classDescription[256] = {};
static SCClass::Selection classSelection;
static int classPreview = -1;
static bool classPreviewActive = false;
static float classPreviewYaw = 0, classPreviewAspect = 2968.f / 3768.f;
static bool classPreviewFace = false;

static void classConfigure() {
    auto entry = (SC_GetClassProfile)GetProcAddress(adapterModule, "SoulsCharacterGetClassProfile");
    auto p = entry ? entry(SC_CLASS_PROFILE_VERSION, sizeof(SC_ClassProfile)) : nullptr;
    classEnabled = GetPrivateProfileIntA("Framework", "StartingClass", 1, ini) && SCClass::valid(p);
    if (classEnabled) {
        classProfile = *p;
        classEnabled =
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, p->name, -1, className, 64) > 0 &&
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, p->description, -1, classDescription,
                                256) > 0;
    }
}

static int classNativeSlot() {
    return 7;
} // PTDE firecaster = Pyromancer.

static bool classCreationMenu() {
    uintptr_t menu = read<uint32_t>(0x13786d0);
    // Native new-character menu +94; no world actor may coexist with a preview.
    int phase = read<int>(menu + 0x94);
    return classEnabled && classHooksReady && menu && !chain(playerSlot, {0, 4, 0}) &&
           (phase == 2 || phase == 3 || phase == 5 || phase == 7 ||
            (read<int>(menu + 0xb4) == 1 && read<int>(menu + 0xb0) > 0 &&
             read<int>(menu + 0xb0) < 16));
}

static bool classRead(bool &matches) {
    uintptr_t stats = chain(0x1378700, {0, 8});
    uint8_t slot = 255;
    bool known = stats && get(stats + 0xc6, slot) && slot < 10;
    matches = known && slot == classNativeSlot();
    return known;
}

static bool classSelected() {
    bool matches = false;
    return classRead(matches) && matches;
}

extern "C" {
void *classOriginalText = nullptr;
}

extern "C" __attribute__((used, noinline)) uintptr_t classText(int id, uintptr_t fmg) {
    if (!classEnabled || !classHooksReady)
        return 0;
    uintptr_t repo = read<uint32_t>(0x13785dc);
    if (!repo || (fmg != read<uint32_t>(repo + 0x138) && fmg != read<uint32_t>(repo + 0x1f0)))
        return 0;
    if (id == 132020 + classNativeSlot())
        return (uintptr_t)className;
    if (id == 132320 + classNativeSlot())
        return (uintptr_t)classDescription;
    return 0;
}

__attribute__((naked)) static void classTextHook() {
    __asm__ volatile(
        "pushfl\n pushal\n movl 40(%esp),%eax\n pushl %eax\n pushl %edx\n call _classText\n addl $8,%esp\n testl %eax,%eax\n jz 1f\n movl %eax,28(%esp)\n popal\n popfl\n ret\n 1: popal\n popfl\n jmp *_classOriginalText\n");
}

// Native class initialization is deliberately untouched: tutorial pickups,
// attributes, inventory and progression use the original Pyromancer records.
static void classInstall() {
    if (!classEnabled)
        return;
    const unsigned char signature[] = {0x53, 0x8b, 0x5c, 0x24, 0x08, 0x55, 0x56};
    unsigned char actual[sizeof(signature)] = {};
    if (!get(uintptr_t(0xd175e0), actual) || memcmp(actual, signature, sizeof(actual))) {
        logf("CLASS text signature refused");
        return;
    }
    auto result =
        MH_CreateHook((void *)0xd175e0, (void *)classTextHook, (void **)&classOriginalText);
    classHooksReady = result == MH_OK && MH_EnableHook((void *)0xd175e0) == MH_OK;
    logf("CLASS native text hook=%d ready=%d; native starting inventory retained", result,
         classHooksReady);
}

static void classPreviewRelease() {
    if (classPreview >= 0) {
        guest->destroy(classPreview);
        classPreview = -1;
        guest->surfaces(nullptr, 0, 0);
        characterMesh.view.numTrianglesUsed = 0;
        logf("CLASS preview released");
    }
    classPreviewActive = false;
}

static void classTick() {
    bool wanted = classCreationMenu() && classSelected();
    if (!wanted)
        classPreviewRelease();
    if (wanted && !characterOwned() && character < 0) {
        if (classPreview < 0) {
            // Preview-only floor. No world physics, health, input capture or native actor.
            SC_Surface floor = {};
            floor.vertices[0][0] = -1000;
            floor.vertices[0][2] = -1000;
            floor.vertices[1][2] = 1000;
            floor.vertices[2][0] = 1000;
            floor.vertices[2][2] = -1000;
            guest->surfaces(&floor, 1, 0);
            classPreview = guest->create(0, 0, 0, 0);
            logf("CLASS preview created=%d", classPreview);
        }
        static DWORD last = 0;
        DWORD now = GetTickCount();
        if (classPreview >= 0 && now - last >= 33) {
            last = now;
            uintptr_t viewer = chain(0x13783c0, {0, 0x6c0, classPreviewFace ? 0x10u : 0xcu});
            float angle = read<float>(viewer + 0x1b4);
            classPreviewYaw = classProfile.previewYaw + (isfinite(angle) ? angle : 0);
            SC_Pose pose = {};
            pose.phase = -1;
            pose.yaw = classPreviewYaw;
            classPreviewActive = guestPose(classPreview, &pose, &state, &characterMesh.view);
            if (!classPreviewActive)
                classPreviewRelease();
        }
    }
    // Saved-class identity must not depend on the cosmetic class-name text hook.
    bool matches = false, known = classRead(matches);
    auto decision = classSelection.observe(player, known, classEnabled && matches, currentMap(),
                                           player && read<int>(player + 0x2d4) > 0);
    if (decision == SCClass::Decision::Enable) {
        if (!characterControl.session.requested())
            logf("CLASS saved identity restored character=%s", profile->id);
        characterControl.session.request(true);
    } else if (decision == SCClass::Decision::Disable) {
        characterControl.session.request(false);
    }
}
