// DLL composition root: native bindings, character lifecycle, simulation and
// Direct3D hooks. Internal feature headers share this translation unit's state;
// keep their dependency order explicit until that state has an owning type.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <mmsystem.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include "../../sdk/character_api.h"
#include "../../core/validation.h"
#include "../../../work/minhook/include/MinHook.h"
#include "memory.h"
#include "character_state.h"
#include "controller.h"
#include "interaction.h"
#include "../../core/combat.h"
#include "../../core/feedback.h"
#include "../../core/motion.h"
#include "../../core/session.h"
#include "../../core/context_action.h"
#include "../../core/diagnostics.h"
static void diagnosticEvent(SCDiagnostics::Kind kind, int before = 0, int after = 0);
static bool diagnosticEnabled = true;
static void combatReset();
static bool deathBegin(double &acc);
static int currentMap();
static void presentationRelease();
static void presentationValidate();
static void launcherConfigure();
static void launcherCancel();
static bool launcherVisible();
static void equipmentReset();
static void feedbackReset();
static void feedbackSettings();
static void actionReset();
static void combatSettings();
static bool combatTestDrop(int height);
static int combatNativeStamina(uintptr_t actor, int value, uintptr_t caller, uintptr_t packet);

static HMODULE selfModule;
static char root[MAX_PATH], ini[MAX_PATH];
static FILE *logfile;

static void logf(const char *fmt, ...) {
    if (!logfile)
        return;
    va_list a;
    va_start(a, fmt);
    vfprintf(logfile, fmt, a);
    va_end(a);
    fputc('\n', logfile);
    fflush(logfile);
}

// These short names are local to the host implementation; adapters never receive
// pointers or access to the native-memory helpers.
using SCNativeMemory::chain;
using SCNativeMemory::get;
using SCNativeMemory::put;
using SCNativeMemory::read;

// Accept an address only when its signature is unique in executable sections.
// A mismatched executable therefore leaves the binding unavailable.
static uintptr_t scan(const char *sig, int immediate) {
    std::vector<int> pat;
    const char *s = sig;
    while (*s) {
        if (*s == ' ') {
            s++;
            continue;
        }
        if (*s == '?') {
            pat.push_back(-1);
            s++;
        } else {
            char *e;
            pat.push_back(strtoul(s, &e, 16));
            s = e;
        }
    }
    auto base = (BYTE *)GetModuleHandleA(NULL);
    auto dos = (IMAGE_DOS_HEADER *)base;
    auto nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    auto sec = IMAGE_FIRST_SECTION(nt);
    uintptr_t hit = 0;
    int count = 0;
    for (unsigned j = 0; j < nt->FileHeader.NumberOfSections; j++)
        if (sec[j].Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            BYTE *b = base + sec[j].VirtualAddress;
            size_t len = sec[j].Misc.VirtualSize;
            for (size_t i = 0; i + pat.size() <= len; i++) {
                bool ok = true;
                for (size_t k = 0; k < pat.size(); k++)
                    if (pat[k] >= 0 && b[i + k] != pat[k]) {
                        ok = false;
                        break;
                    }
                if (ok) {
                    hit = (uintptr_t)(b + i);
                    count++;
                }
            }
        }
    logf("signature matches=%d at=%08x", count, (unsigned)hit);
    return count == 1 ? read<uint32_t>(hit + immediate) : 0;
}

struct Vec {
    float x, y, z;

    Vec operator+(Vec b) const {
        return {x + b.x, y + b.y, z + b.z};
    }

    Vec operator-(Vec b) const {
        return {x - b.x, y - b.y, z - b.z};
    }

    Vec operator*(float s) const {
        return {x * s, y * s, z * s};
    }
};

static void debugContact(Vec at, bool confirmed);

static float dot(Vec a, Vec b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec cross(Vec a, Vec b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static Vec unit(Vec v) {
    float n = sqrtf(dot(v, v));
    return n > .0001f ? v * (1 / n) : Vec{0, 0, 1};
}

static uintptr_t playerSlot, cameraSlot, mapSlot, player, lastPlayer, posData, cam, lastMap;
static int loadedMap = -1, lastHP = 0;
static unsigned pendingNativeHurt = 0;
static uint32_t savedFlags1, savedFlags2, savedMapFlags;
static float savedOpacity = 1.f;
static bool hideNative = false, disableMapHit = false;
static int renderMode = 1;
static int depthTest = 1, testFrames = 0, testButtons = 0, graphFrames = 0;
static bool combatEnabled = false;
static float testX = 0, testY = 0;
static bool ready = false, showCharacter = true, nativeReaction = false,
            equipmentActionOwned = false;
static DWORD nativeReactionIncomingUntil = 0;
static HostCharacter::ControlState characterControl;
static bool presentationActive = false;

static bool characterOwned() {
    return characterControl.session.visible();
}

static bool characterPresented() {
    return characterOwned() || presentationActive;
}

static bool nativeActionOwned() {
    return characterControl.session.action() || characterControl.session.death();
}

static Vec interactionPrevious;
static int interactionMap = -1;
static SCController::Reader controllerReader;
#include "action_input.h"
static volatile LONG controllerOwnedSlot = -1, controllerNativePolls = 0;
#include "feedback_rumble.h"
static thread_local bool controllerReadRaw = false;

static SCController::Sample pollController(DWORD now) {
    controllerReadRaw = true;
    auto result = controllerReader.poll(now);
    controllerReadRaw = false;
    return result;
}

using ControllerGetState = DWORD(WINAPI *)(DWORD, XINPUT_STATE *);
static ControllerGetState originalControllerState = nullptr, originalControllerStateEx = nullptr;
static SRWLOCK controllerPacketLock = SRWLOCK_INIT;
static XINPUT_STATE controllerPackets[XUSER_MAX_COUNT] = {};
static bool controllerPacketSeen[XUSER_MAX_COUNT] = {};

static DWORD controllerResult(DWORD result, DWORD slot, XINPUT_STATE *output) {
    if (!controllerReadRaw && result == ERROR_SUCCESS && output) {
        if (LONG(slot) == InterlockedCompareExchange(&controllerOwnedSlot, -1, -1)) {
            actionFilterPad(output->Gamepad);
            InterlockedIncrement(&controllerNativePolls);
        }
        // Filtering and the synthetic interact press must change the packet number
        // even when the physical controller has not moved between native polls.
        if (slot < XUSER_MAX_COUNT) {
            AcquireSRWLockExclusive(&controllerPacketLock);
            auto &previous = controllerPackets[slot];
            if (!controllerPacketSeen[slot]) {
                previous = *output;
                controllerPacketSeen[slot] = true;
            } else if (memcmp(&previous.Gamepad, &output->Gamepad, sizeof(XINPUT_GAMEPAD)) != 0) {
                previous.Gamepad = output->Gamepad;
                ++previous.dwPacketNumber;
            }
            output->dwPacketNumber = previous.dwPacketNumber;
            ReleaseSRWLockExclusive(&controllerPacketLock);
        }
    }
    return result;
}

static DWORD WINAPI controllerState(DWORD slot, XINPUT_STATE *output) {
    return controllerResult(originalControllerState(slot, output), slot, output);
}

static DWORD WINAPI controllerStateEx(DWORD slot, XINPUT_STATE *output) {
    return controllerResult(originalControllerStateEx(slot, output), slot, output);
}

static void installControllerHooks() {
    controllerReader.poll(GetTickCount());
    void *target = controllerReader.function();
    if (!target) {
        logf("XInput unavailable; keyboard remains enabled");
        return;
    }
    HMODULE module = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)target, &module);
    auto status = MH_CreateHook(target, (void *)controllerState, (void **)&originalControllerState);
    if (status == MH_OK) {
        controllerReader.setFunction((void *)originalControllerState);
        status = MH_EnableHook(target);
    }
    logf("controller input hook status=%d", status);
    void *extra = module ? (void *)GetProcAddress(module, (LPCSTR)100) : nullptr;
    if (extra && extra != target) {
        status =
            MH_CreateHook(extra, (void *)controllerStateEx, (void **)&originalControllerStateEx);
        if (status == MH_OK)
            status = MH_EnableHook(extra);
        logf("controller extended input hook status=%d", status);
    }
    feedbackInstallRumble(module);
}

static int character = -1;
static Vec origin;
static SC_State state{};
static const SC_CharacterAPI *guest = nullptr;
static const SC_Descriptor *profile = nullptr;
static HMODULE adapterModule = nullptr;
static HostCharacter::MeshBuffers characterMesh;
static std::vector<Vec> mapVertices;
static IDirect3DTexture9 *tex = nullptr;
#include "render_surfaces.h"
static HWAVEOUT audioOut = nullptr;
static WAVEHDR audioHeaders[4] = {};
static int16_t audioBuffers[4][2176] = {};

static void pumpAudio() {
    if (!guest || !(profile->capabilities & SC_CAP_AUDIO))
        return;
    unsigned queued = 0;
    int freeIndex = -1;
    for (int i = 0; i < 4; i++) {
        if (audioHeaders[i].dwFlags & WHDR_INQUEUE)
            queued += audioHeaders[i].dwBufferLength / 4;
        else if (freeIndex < 0)
            freeIndex = i;
    }
    if (freeIndex < 0)
        return;
    unsigned n = guest->audio(queued, 1100, audioBuffers[freeIndex], 1088);
    if (audioOut && n && n <= 1088) {
        audioHeaders[freeIndex].dwBufferLength = n * 4;
        waveOutWrite(audioOut, &audioHeaders[freeIndex], sizeof(WAVEHDR));
    }
}

static bool guestStep(int id, const SC_Input *input, SC_State *s, SC_Mesh *m) {
    return guest->step(id, input, s, m) && SC::frame(*s, *m, *profile);
}

static bool guestPose(int id, const SC_Pose *p, SC_State *s, SC_Mesh *m) {
    return guest->pose(id, p, s, m) && SC::frame(*s, *m, *profile);
}

#include "body.h"
#include "hazards.h"
#include "world.h"
#include "character_visibility.h"
using DamageFn = int(WINAPI *)(int, int, int, float, float, float);
static DamageFn nativeDamage = nullptr;
#include "native_targets.h"
#include "equipment_native.h"
#include "starting_class.h"
#include "native_combat.h"
#include "aim_camera.h"

static Vec cameraEye() {
    return cameraFrameVector(false);
}

static Vec cameraForward() {
    return cameraFrameVector(true);
}

#include "camera.h"
#include "projectile.h"

static int currentMap() {
    uintptr_t p = read<uint32_t>(mapSlot);
    if (!p)
        return -1;
    int world = read<uint8_t>(p + 0xa13), area = read<uint8_t>(p + 0xa12);
    return world >= 10 && world <= 18 && area <= 3 ? world * 100 + area : -1;
}

#include "arena_floor.h"

static bool loadMap() {
    int map = currentMap();
    if (map < 0)
        return false;
    if (map == loadedMap && !mapVertices.empty())
        return true;
    char file[MAX_PATH];
    GetPrivateProfileStringA("MarioSouls", "Collision", "", file, sizeof(file), ini);
    char *slash = strrchr(file, '\\');
    if (!slash)
        return false;
    snprintf(slash + 1, sizeof(file) - (slash + 1 - file), "m%02d_%02d_00_00.bin", map / 100,
             map % 100);
    char prepared[MAX_PATH] = {}, directory[MAX_PATH] = {};
    GetPrivateProfileStringA("Framework", "CollisionDirectory", "", directory, sizeof(directory),
                             ini);
    FILE *f = nullptr;
    if (directory[0]) {
        snprintf(prepared, sizeof(prepared), "%s\\m%02d_%02d_00_00.bin", directory, map / 100,
                 map % 100);
        f = fopen(prepared, "rb");
        if (f)
            logf("Native-owned event collision cache: %s", prepared);
    }
    if (!f)
        f = fopen(file, "rb");
    if (!f) {
        logf("No collision cache for map %d", map);
        return false;
    }
    char magic[4];
    uint32_t count = 0;
    bool ok = fread(magic, 1, 4, f) == 4;
    bool typed = ok && !memcmp(magic, "MSC2", 4);
    ok = ok && (typed || !memcmp(magic, "MSC1", 4)) && fread(&count, 4, 1, f) == 1 && count > 0 &&
         count < 2000000;
    std::vector<Vec> data;
    std::vector<SCHazard::Triangle> hazards;
    if (ok)
        for (unsigned i = 0; i < count; i++) {
            uint32_t filter = 8;
            Vec triangle[3];
            if ((typed && fread(&filter, 4, 1, f) != 1) || filter > 255 ||
                fread(triangle, sizeof(Vec), 3, f) != 3) {
                ok = false;
                break;
            }
            for (auto p : triangle)
                if (!isfinite(dot(p, p)))
                    ok = false;
            if (!ok)
                break;
            if (hazardLethal(filter)) {
                SCHazard::Triangle t;
                for (int v = 0; v < 3; v++)
                    t.p[v] = {triangle[v].x, triangle[v].y, triangle[v].z};
                hazards.push_back(t);
            }
            // Retain terminal surfaces for guest floor queries (some engines reject
            // movement without a floor below). Contact is fatal before combat/another step.
            // Camera-only trigger surfaces must not stop the fall above the kill plane.
            if (guestTerrainFilter(filter))
                data.insert(data.end(), triangle, triangle + 3);
        }
    if (ok && fgetc(f) != EOF)
        ok = false;
    fclose(f);
    if (!ok || data.empty())
        return false;
    mapVertices.swap(data);
    mapHazards.swap(hazards);
    hazardPreviousValid = false;
    logf("Map hazard metadata=%d lethal triangles=%u", typed, unsigned(mapHazards.size()));
    loadedMap = map;
    arenaLoad();
    arenaRefresh();
    logf("map=%d collision triangles=%u", map, count);
    return true;
}

static void rebuildSurfaces() {
    std::vector<SC_Surface> surfaces;
    surfaces.reserve(mapVertices.size() / 3);
    bool flip = GetPrivateProfileIntA("MarioSouls", "FlipWinding", 1, ini) != 0;
    for (size_t i = 0; i < mapVertices.size(); i += 3) {
        Vec center = (mapVertices[i] + mapVertices[i + 1] + mapVertices[i + 2]) * (1.f / 3);
        if (fabsf(center.x - origin.x) > 110 || fabsf(center.z - origin.z) > 110 ||
            fabsf(center.y - origin.y) > 110)
            continue;
        SC_Surface surface = {};
        for (int v = 0; v < 3; v++) {
            Vec q = (mapVertices[i + (flip ? 2 - v : v)] - origin) * 100;
            surface.vertices[v][0] = (int)roundf(q.x);
            surface.vertices[v][1] = (int)roundf(q.y);
            surface.vertices[v][2] = (int)roundf(q.z);
        }
        surfaces.push_back(surface);
    }
    guest->surfaces(surfaces.data(), surfaces.size(), 0);
}

#include "loader.h"

static void releaseControl(bool preserveRequest = true) {
    presentationRelease();
    hazardPreviousValid = false;
    launcherCancel();
    projectileRateRelease();
    cameraRelease();
    nativeReaction = false;
    nativeReactionIncomingUntil = 0;
    equipmentActionOwned = false;
    characterControl.session.request(preserveRequest && characterControl.session.requested());
    visibilityRelease();
    bodyRelease();
    worldRelease();
    characterControl.clock.clear();
    characterControl.input.clear();
    pendingNativeHurt = 0;
    combatReset();
    actionReset();
    if (guest)
        guest->surfaces(nullptr, 0, 1);
    actionCancelPulse();
    InterlockedExchange(&actionRoute, LONG(ActionRoute::Free));
    InterlockedExchange(&controllerOwnedSlot, -1);
    if (characterOwned() && lastPlayer && chain(playerSlot, {0, 4, 0}) == lastPlayer &&
        read<uint32_t>(lastPlayer + 0x28) == lastMap) {
        uint32_t f1 = read<uint32_t>(lastPlayer + 0x1fc), f2 = read<uint32_t>(lastPlayer + 0x3c4);
        put(lastPlayer + 0x1fc, (f1 & ~0x804000u) | (savedFlags1 & 0x804000));
        put(lastPlayer + 0x3c4, (f2 & ~0x300u) | (savedFlags2 & 0x300));
        if (disableMapHit || worldMode == 1)
            put(lastMap + 0xc4,
                (read<uint32_t>(lastMap + 0xc4) & ~0x10u) | (savedMapFlags & 0x10u));
        if (hideNative)
            put(lastPlayer + 0x258, savedOpacity);
    }
    if (characterOwned() && lastPlayer && chain(playerSlot, {0, 4, 0}) == lastPlayer && hideNative)
        put(lastPlayer + 0x258, savedOpacity);
    characterControl.session.release();
    if (audioOut)
        waveOutReset(audioOut);
    if (character >= 0) {
        guest->destroy(character);
        character = -1;
    }
    characterMesh.view.numTrianglesUsed = 0;
    logf("native controls restored; character requested=%d", characterControl.session.requested());
}

static bool activate() {
    presentationRelease();
    classPreviewRelease();
    if (!player || !posData || !cam || read<int>(player + 0x2d4) <= 0 || !ready || !loadMap())
        return false;
    if (!actionInputReady) {
        logf("Character refused: native action input filter unavailable");
        return false;
    }
    if (GetPrivateProfileIntA("World", "NativeMotion", 1, ini) != 1 || !worldMotionReady) {
        logf("Character refused: native collision bridge required");
        return false;
    }
    hazardPreviousValid = false;
    origin = read<Vec>(posData + 0x10);
    if (!isfinite(origin.x) || !isfinite(origin.y) || !isfinite(origin.z))
        return false;
    rebuildSurfaces();
    float floor = guest->floorHeight(0, 100, 0);
    logf("activate origin=%.3f %.3f %.3f map=%d floor=%.3f", origin.x, origin.y, origin.z,
         loadedMap, floor);
    // Missing static terrain is not an activation veto: native Havok owns
    // event-controlled floors and airborne spawns. Never snap to a cached floor.
    character = guest->create(0, 0, 0, read<float>(posData + 4) - 3.14159265f);
    if (character < 0)
        return false;
    combatReset();
    guest->nativeDamage(character, 0);
    state = {};
    state.attackId = state.interruptedAttackId = -1;
    state.position[1] = 0;
    savedFlags1 = read<uint32_t>(player + 0x1fc);
    savedFlags2 = read<uint32_t>(player + 0x3c4);
    lastPlayer = player;
    lastMap = read<uint32_t>(player + 0x28);
    savedMapFlags = read<uint32_t>(lastMap + 0xc4);
    savedOpacity = read<float>(player + 0x258);
    hideNative = (classSelection.required() ||
                  GetPrivateProfileIntA("MarioSouls", "HideNative", 0, ini) != 0) &&
                 savedOpacity >= 0 && savedOpacity <= 1;
    disableMapHit = GetPrivateProfileIntA("MarioSouls", "DisableMapHit", 1, ini) != 0;
    logf("native opacity=%.3f mapflags=%08x", savedOpacity, savedMapFlags);
    lastHP = read<int>(player + 0x2d4);
    characterControl.session.activate({player, posData, lastMap, loadedMap});
    if (!bodyCapture()) {
        logf("BODY validation failed; native handback");
        releaseControl();
        return false;
    }
    worldActivate();
    visibilityCapture();
    characterControl.clock.clear();
    characterControl.input.clear();
    SC_Pose initial = {};
    initial.yaw = read<float>(posData + 4) - 3.14159265f;
    initial.phase = -1;
    if (!guestPose(character, &initial, &state, &characterMesh.view)) {
        releaseControl();
        return false;
    }
    if (hideNative) {
        put(player + 0x258, 0.f);
        put(player + 0x1fc, read<uint32_t>(player + 0x1fc) & ~0x800000u);
    }
    return true;
}

static bool pressed(int key) {
    static bool prev[256] = {};
    bool down = (GetAsyncKeyState(key) & 0x8000) != 0;
    bool hit = down && !prev[key];
    prev[key] = down;
    return hit;
}

static HostInteraction::Observation nativeInteractionState() {
    HostInteraction::Observation o;
    uintptr_t movement = read<uint32_t>(player + 0x28), tracks = read<uint32_t>(movement + 0x18),
              menu = read<uint32_t>(0x13786d0);
    if (!player || !movement || !tracks || !menu)
        return o;
    int id = 0;
    uint32_t token = 0;
    if (!get(tracks + 14 * 0x9c, id) || !get(tracks + 14 * 0x9c + 4, token))
        return o;
    o.valid = true;
    o.idle = HostInteraction::idleAnimation(id, token);
    for (int i = 0; i < 29; i++) {
        if (!get(tracks + i * 0x9c, id) || !get(tracks + i * 0x9c + 4, token)) {
            o.valid = false;
            return o;
        }
        if (token == 0xffffffff || id < 0)
            continue;
        int anim = id % 100000;
        if (anim >= 7000 && anim < 7100) {
            o.ladder = true;
            o.animation = anim;
        } else if (anim >= 7000 && anim < 9000) {
            o.event = true;
            if (!o.ladder)
                o.animation = anim;
        }
    }
    // Same MenuMan fields inspected by native IsShowMenu (0xC2ABE0).
    // ItemGetMenuManImpl (C22780/C22910) owns +118 while its item queue popup is open.
    // Exclude action prompts (+58) and banner messages (+F0), which do not own controls.
    const unsigned menuFields[] = {0x24, 0x28, 0x3c,  0x40,  0x48,  0x4c,  0x50,  0x5c,
                                   0x60, 0x64, 0x68,  0x70,  0x78,  0x80,  0x84,  0x88,
                                   0x98, 0x9c, 0xa0,  0xac,  0xbc,  0xc4,  0xd8,  0xe8,
                                   0xec, 0xfc, 0x108, 0x118, 0x11c, 0x120, 0x210, 0x2c4};
    for (auto offset : menuFields)
        if (read<uint32_t>(menu + offset)) {
            o.menu = true;
            break;
        }
    Vec local = (read<Vec>(posData + 0x10) - origin) * 100;
    float floor = guest->floorHeight(local.x, local.y + 30, local.z);
    o.grounded = isfinite(floor) && fabsf(floor - local.y) < 35;
    return o;
}

#include "actions.h"

static bool lineClear(Vec a, Vec b) {
    Vec direction = b - a;
    for (size_t i = 0; i < mapVertices.size(); i += 3) {
        Vec v0 = mapVertices[i], e1 = mapVertices[i + 1] - v0, e2 = mapVertices[i + 2] - v0;
        Vec h = cross(direction, e2);
        float determinant = dot(e1, h);
        if (fabsf(determinant) < 1e-7f)
            continue;
        float inv = 1 / determinant;
        Vec s = a - v0;
        float u = inv * dot(s, h);
        if (u < 0 || u > 1)
            continue;
        Vec q = cross(s, e1);
        float v = inv * dot(direction, q);
        if (v < 0 || u + v > 1)
            continue;
        float t = inv * dot(e2, q);
        if (t > .02f && t < .98f)
            return false;
    }
    return true;
}

#include "harness.h"
#include "feedback.h"
#include "combat.h"
#include "object_combat.h"
#include "equipment.h"
#include "qa_arsenal.h"
#include "native_reaction.h"
#include "equipment_actions.h"
#include "death.h"
#include "debug_view.h"
#include "launcher.h"
#include "presentation.h"
#include "diagnostics.h"

// One render-frame observation is shared by every phase. Input routing may
// consume jump/interact edges, so later simulation sees the routed controller.
struct GameplayFrame {
    bool focus;
    DWORD inputTime;
    SCController::Sample pad;
    SCController::Input controller;
    bool padToggle;
    SCSession::Identity identity;
};

static GameplayFrame pollGameplayFrame() {
    player = chain(playerSlot, {0, 4, 0});
    cam = chain(cameraSlot, {0, 0x3c, 0x60});
    uintptr_t mp = read<uint32_t>(player + 0x28);
    posData = read<uint32_t>(mp + 0x1c);
    presentationValidate();
    classTick();
    if (characterOwned() && !characterControl.session.requested())
        releaseControl(false);
    DWORD fg = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &fg);
    bool focus = fg == GetCurrentProcessId();
    debugInput(focus);
    feedbackFocused = focus;
    if (!focus)
        feedbackReset();
    static SCController::Toggle controllerToggle;
    DWORD inputTime = GetTickCount();
    auto pad = pollController(inputTime);
    auto controller = pad.connected && focus ? SCController::map(pad.pad) : SCController::Input{};
    static int lastController = -2;
    if (pad.slot != lastController) {
        logf("controller %s slot=%d", pad.connected ? "connected" : "disconnected", pad.slot);
        lastController = pad.slot;
    }
    harnessTick(focus, pad.connected);
    bool padToggle = controllerToggle.update(pad, focus, inputTime);
    if (padToggle)
        logf("controller Character toggle");
    SCSession::Identity identity = {player, posData, mp, currentMap()};
    return {focus, inputTime, pad, controller, padToggle, identity};
}

// Resolve map/actor replacement and death before any input reaches an adapter.
static bool updateCharacterLifecycle(const GameplayFrame &frame, double &acc) {
    if (characterControl.session.character() && !nativeReaction && !equipmentActionOwned && cam &&
        !characterControl.session.matches(frame.identity) &&
        characterControl.session.sameActor(frame.identity)) {
        if (loadMap() && characterControl.session.changeMap(frame.identity)) {
            rebuildSurfaces();
            worldActivate();
            logf("SESSION seamless map=%d; character rig, movement and attack windows preserved",
                 frame.identity.map);
        }
    }
    if (characterOwned() && (!characterControl.session.matches(frame.identity) || !cam)) {
        logf("SESSION native actor changed; clearing pending work");
        releaseControl(true);
    }
    if (characterOwned() && read<int>(player + 0x2d4) <= 0 && !characterControl.session.death()) {
        launcherCancel();
        if (!deathBegin(acc))
            releaseControl(true);
    }
    if (characterControl.session.death() && read<int>(player + 0x2d4) > 0) {
        logf("Death presentation complete: native respawn");
        releaseControl(true);
        acc = 0;
    }
    if ((frame.focus && pressed(VK_F6)) || frame.padToggle) {
        classSelection.manual(player);
        // A pending load request is not a visible character. Pressing the toggle
        // while still human should enable the guest, not cancel automatic activation.
        if (characterPresented()) {
            characterControl.session.request(false);
            releaseControl(false);
        } else {
            characterControl.session.request(true);
            if (!activate())
                characterControl.session.failed(frame.inputTime);
        }
    }
    cameraTick();
    if (!equipmentActionOwned || characterControl.session.death())
        projectileRateRelease();
    if (characterControl.session.death()) {
        deathTick(acc, frame.pad.slot);
        return false;
    }
    if (!characterOwned() && characterControl.session.requested()) {
        bool safe = frame.focus && frame.identity.valid() && cam && read<int>(player + 0x2d4) > 0;
        if (safe) {
            auto o = nativeInteractionState();
            safe = o.valid && !o.menu && !o.event && !o.ladder &&
                   nativeActiveAllowed(player, false) == 1;
        }
        if (characterControl.session.canRetry(frame.identity, safe, frame.inputTime)) {
            if (activate())
                logf("SESSION character resumed after stable native actor");
            else
                characterControl.session.failed(frame.inputTime);
        }
    }
    return true;
}

// Contextual actions get first refusal on input (for example A at a bonfire).
static void routeGameplayActions(GameplayFrame &frame) {
    static bool previousInteract = false, previousItem = false, previousActionFocus = false;
    static int previousActionSlot = -1;
    bool yDown = frame.pad.connected && (frame.pad.pad.wButtons & XINPUT_GAMEPAD_Y),
         xDown = frame.pad.connected && (frame.pad.pad.wButtons & XINPUT_GAMEPAD_X);
    bool qPressed = pressed('Q'), ePressed = pressed('E');
    bool padEdgeReady = previousActionFocus && previousActionSlot == frame.pad.slot;
    bool padInteract = padEdgeReady && yDown && !previousInteract,
         padItem = padEdgeReady && xDown && !previousItem;
    bool interactPressed = frame.focus && previousActionFocus && (padInteract || qPressed),
         itemPressed = frame.focus && previousActionFocus && (padItem || ePressed);
    previousInteract = yDown;
    previousItem = xDown;
    previousActionFocus = frame.focus;
    previousActionSlot = frame.pad.slot;
    if (!frame.focus) {
        actionCancelPulse();
        equipmentReset();
    }
    if (characterOwned() && !nativeActionOwned() && !equipmentActionOwned) {
        auto native = nativeInteractionState();
        if (native.valid && (native.menu || native.ladder || native.event))
            beginInteraction(frame.pad.slot, false);
    }
    launcherObserve(frame.focus);
    if (launcherBusy() && frame.focus && frame.pad.connected)
        frame.controller = SCController::aim(frame.pad.pad);
    static SCContext::Button contextA;
    auto context =
        contextA.update(frame.controller.jump, frame.focus && frame.pad.connected, player,
                        frame.pad.slot, characterOwned() && !nativeActionOwned() && !nativeReaction,
                        !launcherBusy() && (actionEligible(1) || launcherTarget));
    InterlockedExchange(&actionBlockHeldA, context.blockNative);
    frame.controller.jump = context.jump;
    if (context.interact) {
        if (!launcherInteract())
            requestAction(1, true);
    } else if (interactPressed && launcherBusy())
        launcherCancel();
    else if (interactPressed && launcherInteract()) {
    } else if (interactPressed) {
        if (characterOwned() && !nativeActionOwned())
            requestAction(1, padInteract);
        else if (nativeActionOwned() && actionCurrentRoute() == ActionRoute::Menu)
            actionQueue(1, frame.inputTime, padInteract);
    }
    if (itemPressed && !launcherBusy() && characterOwned() && !nativeActionOwned())
        requestAction(2, padItem);
    if (characterOwned() && !nativeActionOwned() && !actionPending() && actionRequestKind) {
        logf("Action request unanswered; Character retained");
        actionRequestKind = 0;
    }
}

// Test commands retain their focus and dedicated-character guards. Keeping the
// opt-in wire protocol here avoids mixing automation with ordinary input.
static void pollTestCommands(const GameplayFrame &frame) {
    static DWORD configTime = 0;
    static int command = 0;
    if (GetTickCount() - configTime > 200) {
        configTime = GetTickCount();
        int serial = GetPrivateProfileIntA("Test", "Serial", 0, ini);
        if (serial != command) {
            command = serial;
            if (!frame.focus || !harnessIdentity()) {
                logf("test command rejected: focus/identity guard");
            } else {
                int mode = GetPrivateProfileIntA("Test", "Mode", 0, ini);
                if (mode == 5)
                    qaMeleeArsenal();
                if (mode == 6)
                    qaPyromancyKit();
                if (mode == 3)
                    graphFrames = 2;
                if (mode == 1 && !characterOwned()) {
                    if (nativeActionOwned())
                        releaseControl();
                    activate();
                }
                if (mode == 2 && characterPresented()) {
                    classSelection.manual(player);
                    releaseControl(false);
                }
                if (mode == 4 && characterOwned())
                    beginInteraction(frame.pad.slot);
                testFrames =
                    std::clamp<int>(GetPrivateProfileIntA("Test", "Frames", 0, ini), 0, 150);
                testButtons = GetPrivateProfileIntA("Test", "Buttons", 0, ini);
                testX = std::clamp<int>(GetPrivateProfileIntA("Test", "X", 0, ini), -1, 1);
                testY = std::clamp<int>(GetPrivateProfileIntA("Test", "Y", 0, ini), -1, 1);
                logf("test command=%d mode=%d frames=%d buttons=%d", serial, mode, testFrames,
                     testButtons);
            }
        }
    }
}

static void refreshGameplaySettings(const GameplayFrame &frame) {
    static DWORD lastLog = 0;
    if (GetTickCount() - lastLog > 2000) {
        lastLog = GetTickCount();
        combatSettings();
        renderMode = GetPrivateProfileIntA("MarioSouls", "RenderMode", 1, ini);
        depthTest = GetPrivateProfileIntA("MarioSouls", "DepthTest", 1, ini);
        combatEnabled = GetPrivateProfileIntA("MarioSouls", "EnableCombat", 0, ini) != 0;
        Vec p = read<Vec>(posData + 0x10), c = cameraEye();
        logf(
            "state active=%d focus=%d player=%08x cam=%08x xyz=%.3f %.3f %.3f eye=%.3f %.3f %.3f action=%08x tris=%u",
            characterOwned(), frame.focus, (unsigned)player, (unsigned)cam, p.x, p.y, p.z, c.x, c.y,
            c.z, state.rawAction, characterMesh.view.numTrianglesUsed);
    }
}

// Native actions and reactions own their frames. Reconcile the native motion
// acknowledgement before evaluating damage or beginning another guest tick.
static bool prepareCharacterSimulation(const GameplayFrame &frame, double &acc) {
    InterlockedExchange(&controllerOwnedSlot,
                        (characterOwned() || nativeActionOwned()) ? frame.pad.slot : -1);
    if (!nativeActionOwned())
        InterlockedExchange(&actionRoute,
                            LONG(characterOwned() ? ActionRoute::Character : ActionRoute::Free));
    static bool filteredLogged = false;
    if (!filteredLogged && InterlockedCompareExchange(&controllerNativePolls, 0, 0) > 0) {
        logf("controller native input filtering confirmed; right stick preserved");
        filteredLogged = true;
    }
    if (nativeActionOwned()) {
        updateInteraction(acc, frame.focus, frame.pad.slot);
        return false;
    }
    if (!characterOwned()) {
        acc = 0;
        return false;
    }
    if (arenaRefresh())
        rebuildSurfaces();
    if (frame.focus && pressed(VK_F7)) {
        releaseControl();
        if (!activate())
            return false;
    }
    if (equipmentActionsTick(acc, frame.focus))
        return false;
    if (nativeReactionTick(acc, frame.focus))
        return false;
    worldPause(!frame.focus);
    Vec host = read<Vec>(posData + 0x10),
        expected = origin + Vec{state.position[0], state.position[1], state.position[2]} * .01f;
    if (dot(host - expected, host - expected) > 64) {
        logf("native warp/load detected");
        releaseControl(true);
        return false;
    }
    bool resolvedTick = false;
    if (!worldReconcile(resolvedTick)) {
        diagnosticEvent(SCDiagnostics::Kind::MotionRejected);
        logf("WORLD invalid handoff; native controls restored");
        releaseControl();
        return false;
    }
    worldHostSurfaces();
    if (fabsf(state.position[0]) > 4000 || fabsf(state.position[2]) > 4000 ||
        fabsf(state.position[1]) > 5000) {
        Vec shift = {state.position[0], state.position[1], state.position[2]};
        origin = origin + shift * .01f;
        rebuildSurfaces();
        guest->setPosition(character, 0, 0, 0);
        for (size_t i = 0; i < characterMesh.view.numTrianglesUsed * 3; i++) {
            characterMesh.positions[i * 3] -= shift.x;
            characterMesh.positions[i * 3 + 1] -= shift.y;
            characterMesh.positions[i * 3 + 2] -= shift.z;
        }
        state.position[0] = state.position[1] = state.position[2] = 0;
        logf("collision origin rebased");
    }
    feedbackPoll();
    int hp = read<int>(player + 0x2d4);
    bool hurt = hp < lastHP && hp > 0;
    if (hurt) {
        feedbackPause.clear();
        combatReset();
        int lost = lastHP - hp;
        pendingNativeHurt =
            std::max(pendingNativeHurt, lost >= read<int>(player + 0x2d8) / 5 ? 2u : 1u);
        logf("native damage %d -> %d; host-confirmed hurt, no extra i-frames", lastHP, hp);
    }
    lastHP = hp;
    if (resolvedTick && hazardObserve(read<Vec>(posData + 0x10))) {
        environmentKill(acc, "native_kill_surface");
        return false;
    }
    if (resolvedTick && !pendingNativeHurt) {
        if (!combatEnvironment(acc))
            return false;
        combat();
        combatObjects();
        equipmentContact();
        feedbackPoll();
    }
    return true;
}

// Advance the guest at its fixed rate and submit motion for the native host to
// resolve. The next frame consumes that acknowledgement before opening contacts.
static void simulateCharacter(const GameplayFrame &frame, double dt, double &acc) {
    SC_Input input = {};
    Vec look = cameraForward();
    input.camLookX = look.x;
    input.camLookZ = look.z;
    if (frame.focus) {
        input.moveX = ((GetAsyncKeyState('D') & 0x8000) ? 1.f : 0) -
                      ((GetAsyncKeyState('A') & 0x8000) ? 1.f : 0);
        input.moveY = ((GetAsyncKeyState('W') & 0x8000) ? 1.f : 0) -
                      ((GetAsyncKeyState('S') & 0x8000) ? 1.f : 0);
        if (input.moveX == 0 && input.moveY == 0) {
            input.moveX = frame.controller.x;
            input.moveY = frame.controller.y;
        }
        float n = sqrtf(input.moveX * input.moveX + input.moveY * input.moveY);
        if (n > 1) {
            input.moveX /= n;
            input.moveY /= n;
        }
        input.jump = frame.controller.jump || !!(GetAsyncKeyState(VK_SPACE) & 0x8000);
        input.attack = frame.controller.attack || !!(GetAsyncKeyState(VK_LBUTTON) & 0x8000);
        input.crouch = frame.controller.crouch || !!(GetAsyncKeyState(VK_CONTROL) & 0x8000);
        input.guard = frame.controller.guard ||
                      (actionKeyboardGuardReady && !!(GetAsyncKeyState(VK_RBUTTON) & 0x8000));
    }
    SC_Input specialInput = input;
    if (testFrames > 0) {
        specialInput.attack = !!(testButtons & 2);
        specialInput.guard = !!(testButtons & 8);
        specialInput.jump = !!(testButtons & 1);
        specialInput.crouch = !!(testButtons & 4);
    }
    bool parry =
        frame.focus &&
        ((frame.pad.connected && frame.pad.pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) ||
         !!(GetAsyncKeyState(VK_TAB) & 0x8000) || (testFrames > 0 && (testButtons & 16)));
    if (!launcherBusy() &&
        equipmentActionsInput(specialInput, parry, frame.focus, frame.pad.slot)) {
        acc = 1. / SC_FIXED_HZ;
        equipmentActionsTick(acc, frame.focus);
        return;
    }
    if (equipmentActionButtons.suppressAttack())
        input.attack = 0;
    static int lastPadInput = -1;
    static DWORD padLogTime = 0;
    int padInput = (fabsf(frame.controller.x) + fabsf(frame.controller.y) > .01f ? 8 : 0) |
                   (frame.controller.jump ? 1 : 0) | (frame.controller.attack ? 2 : 0) |
                   (frame.controller.crouch ? 4 : 0);
    if (padInput != lastPadInput && DWORD(frame.inputTime - padLogTime) >= 100) {
        logf("controller input stick=%.2f %.2f buttons=%d%d%d", frame.controller.x,
             frame.controller.y, frame.controller.jump, frame.controller.attack,
             frame.controller.crouch);
        lastPadInput = padInput;
        padLogTime = frame.inputTime;
    }
    // Capture short taps even on render frames without a simulation tick. Consume
    // paused time instead of accumulating a catch-up burst. The host keeps running.
    characterControl.input.sample(input, GetTickCount());
    characterControl.clock.advance(dt);
    bool stepped = false;
    Vec stepFrom = origin + Vec{state.position[0], state.position[1], state.position[2]} * .01f;
    if (!frame.focus) {
        characterControl.input.clear();
        characterControl.clock.clear();
        testFrames = 0;
    } else if (SCFeedback::pending(feedbackPause.until, GetTickCount())) {
        feedbackPause.filter(input, GetTickCount());
        characterControl.clock.clear();
        pumpAudio();
    } else if (characterControl.clock.take(worldReady())) {
        input = characterControl.input.take(GetTickCount());
        if (testFrames > 0) {
            input.moveX = -testX;
            input.moveY = -testY;
            input.jump = !!(testButtons & 1);
            input.attack = !!(testButtons & 2);
            input.crouch = !!(testButtons & 4);
            input.guard = !!(testButtons & 8);
            --testFrames;
        }
        harnessInput(input);
        if (equipmentActionButtons.suppressAttack())
            input.attack = 0;
        feedbackPause.filter(input, GetTickCount());
        if (!launcherBusy()) {
            combatInput(input);
            equipmentInput(input);
        }
        uint32_t old = state.rawAction;
        guest->nativeDamage(character, pendingNativeHurt);
        pendingNativeHurt = 0;
        if (!guestStep(character, &input, &state, &characterMesh.view)) {
            diagnosticEvent(SCDiagnostics::Kind::FrameRejected);
            logf("Adapter frame rejected; restoring native controls");
            releaseControl();
            return;
        }
        stepped = true;
        harnessAfterStep(old, input);
        pumpAudio();
        if (launcherApi)
            launcherApi->query(character, &launcherState);
        launcherCameraTick();
        if (old != state.rawAction)
            logf("action %08x -> %08x xyz=%.3f %.3f %.3f buttons=%d%d%d", old, state.rawAction,
                 state.position[0], state.position[1], state.position[2], input.jump, input.attack,
                 input.crouch);
    }
    Vec p = origin + Vec{state.position[0], state.position[1], state.position[2]} * .01f;
    if (p.y < -1000 && isfinite(p.y)) {
        environmentKill(acc, "outside_world");
        return;
    }
    if (!isfinite(p.x) || !isfinite(p.y) || !isfinite(p.z)) {
        logf("invalid/outside test area; disabled");
        releaseControl();
        return;
    }
    actionFlags(false);
    put(lastMap + 0xc4, read<uint32_t>(lastMap + 0xc4) & ~0x10u);
    if (stepped && !worldSubmit(stepFrom, p)) {
        logf("WORLD rejected movement request; native controls restored");
        releaseControl();
        return;
    }
    if (hideNative) {
        put(player + 0x258, 0.f);
        put(player + 0x1fc, read<uint32_t>(player + 0x1fc) & ~0x800000u);
    }
}

static void updateGameplay() {
    static LARGE_INTEGER freq = {}, prev = {};
    if (!freq.QuadPart)
        QueryPerformanceFrequency(&freq);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double dt = prev.QuadPart ? double(now.QuadPart - prev.QuadPart) / freq.QuadPart : 0;
    prev = now;
    static double acc = 0;
    acc = std::min(acc + std::min(dt, .1), .1);

    auto frame = pollGameplayFrame();
    if (!updateCharacterLifecycle(frame, acc))
        return;
    routeGameplayActions(frame);
    pollTestCommands(frame);
    refreshGameplaySettings(frame);
    if (!prepareCharacterSimulation(frame, acc))
        return;
    simulateCharacter(frame, dt, acc);
}

static void update() {
    updateGameplay();
    presentationTick();
    diagnosticsTick();
}

static D3DMATRIX identity() {
    D3DMATRIX m = {};
    m._11 = m._22 = m._33 = m._44 = 1;
    return m;
}

static D3DMATRIX viewMatrix(Vec eye, Vec at) {
    Vec z = unit(at - eye), x = unit(cross({0, 1, 0}, z)), y = cross(z, x);
    D3DMATRIX m = {};
    m._11 = x.x;
    m._12 = y.x;
    m._13 = z.x;
    m._21 = x.y;
    m._22 = y.y;
    m._23 = z.y;
    m._31 = x.z;
    m._32 = y.z;
    m._33 = z.z;
    m._41 = -dot(x, eye);
    m._42 = -dot(y, eye);
    m._43 = -dot(z, eye);
    m._44 = 1;
    return m;
}

struct Vertex {
    float x, y, z;
    DWORD color, shade;
    float u, v;
};

#include "launcher_render.h"

static bool draw(IDirect3DDevice9 *d, bool preview = false) {
    if (preview ? !classPreviewActive : !renderWanted())
        return false;
    bool drawCharacter = (preview || characterPresented()) && characterMesh.view.numTrianglesUsed;
    if (!tex) {
        if (FAILED(d->CreateTexture(profile->textureWidth, profile->textureHeight, 1, 0,
                                    D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, NULL)))
            return false;
        D3DLOCKED_RECT lr;
        if (SUCCEEDED(tex->LockRect(0, &lr, NULL, 0))) {
            for (int y = 0; y < profile->textureHeight; y++) {
                auto row = (uint32_t *)((BYTE *)lr.pBits + y * lr.Pitch);
                for (int x = 0; x < profile->textureWidth; x++) {
                    auto p = &characterMesh.atlasPixels[(y * profile->textureWidth + x) * 4];
                    row[x] = (p[3] << 24) | (p[0] << 16) | (p[1] << 8) | p[2];
                }
            }
            tex->UnlockRect(0);
        }
    }
    IDirect3DStateBlock9 *saved = nullptr;
    if (FAILED(d->CreateStateBlock(D3DSBT_ALL, &saved)))
        return false;
    D3DVIEWPORT9 vp;
    d->GetViewport(&vp);
    Vec eye = cameraEye();
    Vec at = eye + cameraForward();
    if (preview) {
        float height = classProfile.previewHeightCm * .01f;
        eye = {0, height * .55f, -height * 2.f};
        at = {0, height * .5f, 0};
    }
    D3DMATRIX world = identity(), view = viewMatrix(eye, at), proj = {};
    float fov = read<float>(cam + 0x50), aspect = read<float>(cam + 0x54),
          zn = read<float>(cam + 0x58), zf = read<float>(cam + 0x5c);
    if (fov < .1f || fov > 2.8f)
        fov = .75049f;
    if (aspect < .5f || aspect > 5)
        aspect = float(vp.Width) / vp.Height;
    if (zn < .001f || zn > 2)
        zn = .05f;
    if (zf < 50 || zf > 10000)
        zf = 3000;
    if (preview) {
        fov = .65f;
        aspect = classPreviewAspect;
        zn = .01f;
        zf = 100;
    }
    float ys = 1 / tanf(fov / 2);
    proj._11 = ys / aspect;
    proj._22 = ys;
    proj._33 = zf / (zf - zn);
    proj._34 = 1;
    proj._43 = -zn * proj._33;
    Vec renderOrigin = preview ? Vec{} : origin;
    float modelScale = preview ? 1.f : characterBody.modelScale;
    if (!preview && characterOwned() && worldMode == 1 && worldMotionReady) {
        Vec offset = read<Vec>(posData + 0x10) -
                     (origin + Vec{state.position[0], state.position[1], state.position[2]} * .01f);
        if (isfinite(dot(offset, offset)) && dot(offset, offset) < 4)
            renderOrigin = renderOrigin + offset;
    }
    std::vector<Vertex> verts(drawCharacter ? characterMesh.view.numTrianglesUsed * 3 : 0);
    for (size_t i = 0; i < verts.size(); i++) {
        auto &p = verts[i];
        p.x = renderOrigin.x + SCBody::modelCoordinate(characterMesh.positions[i * 3],
                                                       state.position[0], modelScale) *
                                   .01f;
        p.y = renderOrigin.y + SCBody::modelCoordinate(characterMesh.positions[i * 3 + 1],
                                                       state.position[1], modelScale) *
                                   .01f;
        p.z = renderOrigin.z + SCBody::modelCoordinate(characterMesh.positions[i * 3 + 2],
                                                       state.position[2], modelScale) *
                                   .01f;
        int r = std::clamp(int(characterMesh.colors[i * 3] * 255), 0, 255),
            g = std::clamp(int(characterMesh.colors[i * 3 + 1] * 255), 0, 255),
            b = std::clamp(int(characterMesh.colors[i * 3 + 2] * 255), 0, 255);
        p.color = 0xff000000 | (r << 16) | (g << 8) | b;
        Vec normal = unit({characterMesh.normals[i * 3], characterMesh.normals[i * 3 + 1],
                           characterMesh.normals[i * 3 + 2]});
        int light = int(255 * (.4f + .6f * std::max(0.f, dot(normal, unit({-.3f, .8f, -.5f})))));
        p.shade = 0xff000000 | (light << 16) | (light << 8) | light;
        p.u = characterMesh.uvs[i * 2];
        p.v = characterMesh.uvs[i * 2 + 1];
    }
    d->SetVertexShader(NULL);
    d->SetPixelShader(NULL);
    d->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
    d->SetTransform(D3DTS_WORLD, &world);
    d->SetTransform(D3DTS_VIEW, &view);
    d->SetTransform(D3DTS_PROJECTION, &proj);
    d->SetRenderState(D3DRS_LIGHTING, FALSE);
    d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    d->SetRenderState(D3DRS_ZENABLE, (preview || depthTest) ? TRUE : FALSE);
    d->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    d->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    d->SetRenderState(D3DRS_ALPHATESTENABLE,
                      (profile->capabilities & SC_CAP_ALPHA_TEST) ? TRUE : FALSE);
    d->SetRenderState(D3DRS_ALPHAREF, 127);
    d->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    d->SetRenderState(D3DRS_FOGENABLE, FALSE);
    d->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    d->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    d->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
    d->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    d->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    d->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
    d->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    d->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    d->SetTexture(0, tex);
    d->SetTextureStageState(0, D3DTSS_COLOROP,
                            (profile->capabilities & SC_CAP_TEXTURE_MODULATE)
                                ? D3DTOP_MODULATE
                                : D3DTOP_BLENDTEXTUREALPHA);
    d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    d->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    d->SetTextureStageState(0, D3DTSS_ALPHAARG1,
                            (profile->capabilities & SC_CAP_ALPHA_TEST) ? D3DTA_TEXTURE
                                                                        : D3DTA_DIFFUSE);
    d->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    d->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
    d->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
    d->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_SPECULAR);
    d->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    d->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
    d->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
    d->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    d->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    d->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    d->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    HRESULT hr = drawCharacter
                     ? d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, characterMesh.view.numTrianglesUsed,
                                          verts.data(), sizeof(Vertex))
                     : S_OK;
    static bool logged = false;
    if (!logged) {
        logf("draw result=%08lx viewport=%ux%u fov=%.5f near=%.5f far=%.1f", hr, vp.Width,
             vp.Height, fov, zn, zf);
        logged = true;
    }
    if (!preview) {
        if (drawCharacter) {
            equipmentDraw(d, renderOrigin);
            feedbackDraw(d);
        }
        debugDraw(d);
        launcherDraw(d);
    }
    saved->Apply();
    saved->Release();
    return SUCCEEDED(hr);
}

typedef HRESULT(WINAPI *EndSceneFn)(IDirect3DDevice9 *);
static EndSceneFn originalEnd;
#include "render_composite.h"
#include "class_preview.h"
#include "ui_order.h"
static bool updatedForPresent = false;

static HRESULT WINAPI endScene(IDirect3DDevice9 *d) {
    static bool realHooks = false;
    if (!realHooks) {
        realHooks = true;
        installUIHooks(*(void ***)d);
    }
    if (!updatedForPresent) {
        updatedForPresent = true;
        update();
    }
    uiFrameEnd();
    if (renderMode == 0)
        draw(d);
    return originalEnd(d);
}

using ResetFn = HRESULT(WINAPI *)(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *);
static ResetFn originalReset;

static HRESULT WINAPI reset(IDirect3DDevice9 *d, D3DPRESENT_PARAMETERS *p) {
    updatedForPresent = false;
    uiComposited = false;
    classPreviewReset();
    renderSurfacesReset();
    return originalReset(d, p);
}

using PresentFn = HRESULT(WINAPI *)(IDirect3DDevice9 *, const RECT *, const RECT *, HWND,
                                    const RGNDATA *);
using RenderTargetFn = HRESULT(WINAPI *)(IDirect3DDevice9 *, DWORD, IDirect3DSurface9 *);
static PresentFn originalPresent;
static RenderTargetFn originalRenderTarget;
#include "startup.h"
static unsigned rtCount = 0, mainUses = 0;
static IDirect3DSurface9 *mainTarget = nullptr;
static bool sceneDrawn = false;

static HRESULT WINAPI present(IDirect3DDevice9 *d, const RECT *a, const RECT *b, HWND c,
                              const RGNDATA *e) {
    renderDepthEndFrame();
    updatedForPresent = false;
    rtCount = mainUses = 0;
    mainTarget = nullptr;
    sceneDrawn = false;
    if (graphFrames > 0)
        --graphFrames;
    if (skipIntroPresent()) {
        uiPresent();
        return S_OK;
    }
    HRESULT hr = originalPresent(d, a, b, c, e);
    uiPresent();
    return hr;
}

static HRESULT WINAPI setRenderTarget(IDirect3DDevice9 *d, DWORD index, IDirect3DSurface9 *target) {
    if (index == 0) {
        ++rtCount;
        if (rtCount == 1)
            mainTarget = target;
        if (target == mainTarget)
            ++mainUses;
        if (player && !uiInside)
            renderDepthObserve(d);
        if (graphFrames == 1) {
            IDirect3DSurface9 *old = nullptr, *depth = nullptr;
            d->GetRenderTarget(0, &old);
            d->GetDepthStencilSurface(&depth);
            D3DSURFACE_DESC desc = {}, dd = {};
            if (old)
                old->GetDesc(&desc);
            if (depth)
                depth->GetDesc(&dd);
            logf(
                "graph pass=%u mainuses=%u old=%08x new=%08x size=%ux%u format=%u depth=%08x depthsize=%ux%u",
                rtCount, mainUses, (unsigned)(uintptr_t)old, (unsigned)(uintptr_t)target,
                desc.Width, desc.Height, desc.Format, (unsigned)(uintptr_t)depth, dd.Width,
                dd.Height);
            using SaveFn = HRESULT(WINAPI *)(const char *, int, IDirect3DSurface9 *, const void *,
                                             const RECT *);
            auto save =
                (SaveFn)GetProcAddress(GetModuleHandleA("d3dx9_43.dll"), "D3DXSaveSurfaceToFileA");
            if (save && old) {
                char path[MAX_PATH];
                snprintf(path, sizeof(path), "%s\\charactersouls-pass-%02u.bmp", root, rtCount);
                save(path, 0, old, nullptr, nullptr);
            }
            if (old)
                old->Release();
            if (depth)
                depth->Release();
        }
        if (renderWanted() && renderMode >= 100 && rtCount == unsigned(renderMode - 100)) {
            draw(d);
            sceneDrawn = true;
        }
        if (renderWanted() && !sceneDrawn && renderMode == 1 && mainUses == 2 && mainTarget) {
            IDirect3DSurface9 *old = nullptr;
            d->GetRenderTarget(0, &old);
            if (old == mainTarget && target != old) {
                draw(d);
                sceneDrawn = true;
            }
            if (old)
                old->Release();
        }
    }
    return originalRenderTarget(d, index, target);
}

static DWORD WINAPI start(void *) {
    GetModuleFileNameA(selfModule, root, MAX_PATH);
    char *p = strrchr(root, '\\');
    if (p)
        *p = 0;
    snprintf(ini, sizeof(ini), "%s\\mariosouls.ini", root);
    char file[MAX_PATH];
    snprintf(file, sizeof(file), "%s\\mariosouls.log", root);
    logfile = fopen(file, "w");
    logf("Souls Character Framework 0.9.0 host ds1ptde starting");
    if (read<uint32_t>(0x400080) != 0xfc293654) {
        logf("Unsupported executable; inactive");
        return 0;
    }
    playerSlot = scan("8B 15 ? ? ? ? F3 0F 10 44 24 30 52", 2);
    cameraSlot = scan("D9 45 08 A1 ? ? ? ? 51 D9 1C 24 50", 4);
    if (!playerSlot || !cameraSlot)
        return 0;
    mapSlot = scan(
        "8B 48 04 8B 11 50 8B 42 34 FF D0 C7 46 2C 00 00 00 00 C3 8B 0D ? ? ? ? 8B 89 50 0B 00 00 32 C0 83 79 28 00",
        0x15);
    if (!mapSlot)
        return 0;
    eventFlagsSlot = scan("56 8B F1 8B 46 1C 50 A1 ? ? ? ? 32 C9", 8);
    ready = initCharacter();
    if (!ready) {
        logf("initialization failed");
        return 0;
    }
    logf("initializing graphics probe");
    WNDCLASSA wc = {};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = selfModule;
    wc.lpszClassName = "SoulsCharacterProbe";
    RegisterClassA(&wc);
    HWND win = CreateWindowA(wc.lpszClassName, "", WS_OVERLAPPED, 0, 0, 64, 64, NULL, NULL,
                             selfModule, NULL);
    logf("probe window=%08x", (unsigned)(uintptr_t)win);
    IDirect3D9 *d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
        return 0;
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = win;
    IDirect3DDevice9 *d = nullptr;
    HRESULT hr =
        d3d->CreateDevice(0, D3DDEVTYPE_HAL, win, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &d);
    if (SUCCEEDED(hr)) {
        void **table = *(void ***)d;
        MH_Initialize();
        classInstall();
        aimCameraInstall();
        installVisibility();
        if (nativeDamage)
            characterInstallDamage();
        worldInstall();
        projectileInstall();
        installControllerHooks();
        installActionInput();
        installActionEffects();
        installStartupHooks(table);
        MH_STATUS s = MH_CreateHook(table[42], (void *)endScene, (void **)&originalEnd);
        logf("hook create=%d", s);
        if (s == MH_OK)
            MH_EnableHook(table[42]);
        s = MH_CreateHook(table[16], (void *)reset, (void **)&originalReset);
        if (s == MH_OK)
            MH_EnableHook(table[16]);
        s = MH_CreateHook(table[17], (void *)present, (void **)&originalPresent);
        if (s == MH_OK)
            MH_EnableHook(table[17]);
        s = MH_CreateHook(table[37], (void *)setRenderTarget, (void **)&originalRenderTarget);
        if (s == MH_OK)
            MH_EnableHook(table[37]);
        logf("scene hooks ready");
        d->Release();
    } else
        logf("D3D probe failed %08lx", hr);
    d3d->Release();
    DestroyWindow(win);
    return 0;
}

extern "C" __declspec(dllexport) HRESULT WINAPI DirectInput8Create(HINSTANCE h, DWORD v, REFIID r,
                                                                   LPVOID *out, LPUNKNOWN unk) {
    static HMODULE real = nullptr;
    if (!real) {
        char p[MAX_PATH];
        GetSystemDirectoryA(p, MAX_PATH);
        strcat(p, "\\dinput8.dll");
        real = LoadLibraryA(p);
    }
    using Fn = HRESULT(WINAPI *)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
    auto fn = (Fn)GetProcAddress(real, "DirectInput8Create");
    HRESULT hr = fn ? fn(h, v, r, out, unk) : E_FAIL;
    logf("DirectInput forward real=%08x version=%x hr=%08lx", (unsigned)(uintptr_t)real,
         (unsigned)v, hr);
    return hr;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        selfModule = h;
        DisableThreadLibraryCalls(h);
        HANDLE t = CreateThread(NULL, 0, start, NULL, 0, NULL);
        if (t)
            CloseHandle(t);
    }
    return TRUE;
}
