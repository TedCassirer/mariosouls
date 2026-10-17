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
#include "build.generated.h"
#include "../../core/validation.h"
#include "../../core/terrain_policy.h"
#include "../../core/mesh_interpolation.h"
#include "../../core/render_geometry.h"
#include "../../core/host_capabilities.h"
#include "font_atlas.h"
#include "../../../work/minhook/include/MinHook.h"
#include "memory.h"
#include "render_device.h"
static_assert(sizeof(void *) == 4, "PTDE native bindings require a 32-bit host");
#include "character_state.h"
#include "controller.h"
#include "interaction.h"
#include "../../core/combat.h"
#include "../../core/feedback.h"
#include "../../core/motion.h"
#include "../../core/session.h"
#include "../../core/context_action.h"
#include "../../core/flag_lease.h"
static void combatReset();
static bool deathBegin(double &acc);
static int currentMap();
static void presentationRelease();
static void presentationValidate();
static bool cutsceneVisible();
static void launcherConfigure();
static void launcherCancel();
static bool launcherVisible();
static bool launcherBusy();
static SCHost::Capabilities hostCapabilities(bool beforeHUD = false);
static bool hostPermits(SCHost::Operation operation);
static void equipmentReset();
static void feedbackReset();
static void feedbackSettings();
static void actionReset();
static void combatSettings();
static void environmentConfigure();
static void environmentReset();
static int combatNativeStamina(uintptr_t actor, int value, uintptr_t caller, uintptr_t packet);

static HMODULE selfModule;
static char root[MAX_PATH], ini[MAX_PATH];
static FILE *logfile;
static SRWLOCK logLock = SRWLOCK_INIT;
static size_t logBytes = 0;

static void logMessage(const char *fmt, ...) {
    if (!logfile)
        return;
    AcquireSRWLockExclusive(&logLock);
    constexpr size_t limit = 256 * 1024;
    if (logBytes >= limit) {
        ReleaseSRWLockExclusive(&logLock);
        return;
    }
    char message[1024];
    va_list a;
    va_start(a, fmt);
    int count = vsnprintf(message, sizeof(message) - 1, fmt, a);
    va_end(a);
    if (count >= 0) {
        size_t length = std::min(size_t(count), sizeof(message) - 2);
        message[length++] = '\n';
        length = std::min(length, limit - logBytes);
        logBytes += fwrite(message, 1, length, logfile);
        fflush(logfile);
    }
    ReleaseSRWLockExclusive(&logLock);
}

// These short names are local to the host implementation; adapters never receive
// pointers or access to the native-memory helpers.
using SCNativeMemory::chain;
using SCNativeMemory::get;
using SCNativeMemory::put;
using SCNativeMemory::read;

#include "native_bindings.h"

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

static HostBindings::Slots gameSlots;
// Current native observations are refreshed once per gameplay frame. The owned
// snapshot guards restoration: an address must still belong to the same actor.
static uintptr_t player, playerPhysics, playerCamera;
static uintptr_t ownedActor, ownedMovement;
static int loadedMap = -1, lastHP = 0;
static unsigned pendingNativeHurt = 0;
static uint32_t savedFlags1, savedMapFlags;
static SCFlags::Lease actionControlLocks;
static bool hideNative = false, disableMapHit = false;
static int renderMode = 1;
static int depthTest = 1;
static bool combatEnabled = false;
static bool ready = false, nativeReaction = false, equipmentActionOwned = false;
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
#include "action_input.h"
#include "feedback_rumble.h"
#include "controller_hooks.h"

static int character = -1;
static Vec origin;
static SC_State state{};
static const SC_CharacterAPI *guest = nullptr;
static const SC_Descriptor *profile = nullptr;
static HMODULE adapterModule = nullptr;
static HostCharacter::MeshBuffers characterMesh;
static SCVisual::MeshHistory visualMesh;
static SCVisual::Anchor visualAnchor;
static float visualAlpha = 1;
static bool visualFocused = false;

static double visualTime() {
    static LARGE_INTEGER frequency = {};
    if (!frequency.QuadPart)
        QueryPerformanceFrequency(&frequency);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return double(now.QuadPart) / frequency.QuadPart;
}

static void visualReset() {
    visualMesh.clear();
    visualAnchor.clear();
}

static void visualCapture(int id, uint32_t mode, const SC_State &s, const SC_Mesh &m) {
    SC_Attachments hands = {sizeof(SC_Attachments)};
    if (!guest->attachments || !(profile->capabilities & SC_CAP_EQUIPMENT) ||
        !guest->attachments(id, &hands) || hands.size != sizeof(hands) || (hands.valid & ~3u))
        hands = {sizeof(SC_Attachments)};
    visualMesh.capture(m, s.position, hands, visualTime(), (uint64_t(uint32_t(id)) << 32) | mode);
}

static std::vector<Vec> mapVertices;
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
    bool ok = guest->step(id, input, s, m) && SC::frame(*s, *m, *profile);
    if (ok)
        visualCapture(id, 0, *s, *m);
    else
        visualReset();
    return ok;
}

static bool guestPose(int id, const SC_Pose *p, SC_State *s, SC_Mesh *m) {
    bool ok = guest->pose(id, p, s, m) && SC::frame(*s, *m, *profile);
    if (ok)
        visualCapture(id, p->kind + 1, *s, *m);
    else
        visualReset();
    return ok;
}

#include "body.h"
#include "hazards.h"
#include "world.h"
#include "character_visibility.h"
using DamageFn = int(WINAPI *)(int, int, int, float, float, float);
static DamageFn nativeDamage = nullptr;
#include "native_targets.h"
#include "equipment_native.h"
#include "unarmed_equipment.h"
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
#include "defense.h"

static int currentMap() {
    uintptr_t p = read<uint32_t>(gameSlots.map);
    if (!p)
        return -1;
    int world = read<uint8_t>(p + 0xa13), area = read<uint8_t>(p + 0xa12);
    return world >= 10 && world <= 18 && area <= 3 ? world * 100 + area : -1;
}

#include "arena_floor.h"

// Terrain cache and guest-local surface preparation.
#include "terrain.h"

#include "loader.h"
#include "capabilities.h"
static void nativeRecoveryReset();

static void releaseControl(bool preserveRequest = true) {
    visualReset();
    presentationRelease();
    hazardPreviousValid = false;
    launcherCancel();
    projectileRateRelease();
    defenseRateRelease();
    cameraRelease();
    nativeReaction = false;
    nativeReactionIncomingUntil = 0;
    nativeRecoveryReset();
    environmentReset();
    equipmentActionOwned = false;
    characterControl.session.request(preserveRequest && characterControl.session.requested());
    visibilityRelease();
    bodyRelease();
    worldRelease();
    characterControl.resetSimulation();
    pendingNativeHurt = 0;
    combatReset();
    actionReset();
    if (guest)
        guest->surfaces(nullptr, 0, 1);
    actionCancelPulse();
    InterlockedExchange(&actionRoute, LONG(ActionRoute::Free));
    HostController::setOwner(-1);
    if (characterOwned() && ownedActor && chain(gameSlots.player, {0, 4, 0}) == ownedActor &&
        read<uint32_t>(ownedActor + 0x28) == ownedMovement) {
        uint32_t f1 = read<uint32_t>(ownedActor + 0x1fc), f2 = read<uint32_t>(ownedActor + 0x3c4);
        put(ownedActor + 0x1fc, (f1 & ~0x4000u) | (savedFlags1 & 0x4000));
        put(ownedActor + 0x3c4, actionControlLocks.update(f2, 0));
        if (disableMapHit || worldMode == 1)
            put(ownedMovement + 0xc4,
                (read<uint32_t>(ownedMovement + 0xc4) & ~0x10u) | (savedMapFlags & 0x10u));
    }
    characterControl.session.release();
    actionControlLocks.abandon();
    if (audioOut)
        waveOutReset(audioOut);
    if (character >= 0) {
        guest->destroy(character);
        character = -1;
    }
    characterMesh.view.numTrianglesUsed = 0;
    logMessage("native controls restored; character requested=%d",
               characterControl.session.requested());
}

static bool activate() {
    presentationRelease();
    classPreviewRelease();
    if (!player || !playerPhysics || !playerCamera || read<int>(player + 0x2d4) <= 0 || !ready ||
        !loadMap())
        return false;
    if (!hostPermits(SCHost::Operation::Movement)) {
        logMessage("Character refused: movement/health/input facilities unavailable (mask=%x)",
                   hostCapabilities().available);
        return false;
    }
    if (GetPrivateProfileIntA("World", "NativeMotion", 1, ini) != 1 || !worldMotionReady) {
        logMessage("Character refused: native collision bridge required");
        return false;
    }
    hazardPreviousValid = false;
    origin = read<Vec>(playerPhysics + 0x10);
    if (!isfinite(origin.x) || !isfinite(origin.y) || !isfinite(origin.z))
        return false;
    rebuildSurfaces();

    // Missing static terrain is not an activation veto: native Havok owns
    // event-controlled floors and airborne spawns. Never snap to a cached floor.
    character = guest->create(0, 0, 0, read<float>(playerPhysics + 4) - 3.14159265f);
    if (character < 0)
        return false;
    combatReset();
    guest->nativeDamage(character, 0);
    state = {};
    state.attackId = state.interruptedAttackId = -1;
    state.position[1] = 0;
    savedFlags1 = read<uint32_t>(player + 0x1fc);
    actionControlLocks.abandon();
    ownedActor = player;
    ownedMovement = read<uint32_t>(player + 0x28);
    savedMapFlags = read<uint32_t>(ownedMovement + 0xc4);
    hideNative =
        classSelection.required() || GetPrivateProfileIntA("MarioSouls", "HideNative", 0, ini) != 0;
    if (hideNative && !hostPermits(SCHost::Operation::Replacement)) {
        logMessage("Character refused: mesh-only visibility hook unavailable");
        guest->destroy(character);
        character = -1;
        return false;
    }
    disableMapHit = GetPrivateProfileIntA("MarioSouls", "DisableMapHit", 1, ini) != 0;

    lastHP = read<int>(player + 0x2d4);
    characterControl.session.activate({player, playerPhysics, ownedMovement, loadedMap});
    if (!bodyCapture()) {
        logMessage("BODY validation failed; native handback");
        releaseControl();
        return false;
    }
    worldActivate();
    visibilityCapture();
    characterControl.resetSimulation();
    SC_Pose initial = {};
    initial.yaw = read<float>(playerPhysics + 4) - 3.14159265f;
    initial.phase = -1;
    if (!guestPose(character, &initial, &state, &characterMesh.view)) {
        releaseControl();
        return false;
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

static HostInteraction::Observation nativeInteractionState(bool probeFloor = true) {
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
    if (probeFloor) {
        // Native item scripts require Havok support, not a guest floor query.
        // A sloped floor or a native-owned tread may differ from the guest cache.
        if (!worldSupportReady || !PTDESupport::grounded(playerPhysics, o.grounded))
            o.grounded = false;
    }
    return o;
}

#include "actions.h"

#include "feedback.h"
#include "combat.h"
#include "object_combat.h"
#include "equipment.h"
#include "native_reaction.h"
#include "equipment_actions.h"
#include "death.h"
#include "debug_view.h"
#include "launcher.h"
#include "presentation.h"
#include "cutscene.h"
#include "environment.h"

// Frame phases run serially before any character drawing.
#include "gameplay.h"

// PTDE scene/preview drawing; device hooks below preserve the existing order.
#include "render_character.h"

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
    PTDEGraphics::resetTextures();
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

static HANDLE startupComplete;
static bool graphicsBootstrapInstalled = false;
static void installDeviceHooks(IDirect3DDevice9 *device);
#include "graphics_bootstrap.h"

static DWORD WINAPI start(void *) {
    struct Completion {
        ~Completion() {
            SetEvent(startupComplete);
        }
    } completion;

    GetModuleFileNameA(selfModule, root, MAX_PATH);
    char *p = strrchr(root, '\\');
    if (p)
        *p = 0;
    snprintf(ini, sizeof(ini), "%s\\mariosouls.ini", root);
    char file[MAX_PATH];
    snprintf(file, sizeof(file), "%s\\mariosouls.log", root);
    logfile = fopen(file, "wb");
    logMessage("Souls Character Framework %s host ds1ptde starting", SC_BUILD_VERSION);
    if (read<uint32_t>(0x400080) != 0xfc293654) {
        logMessage("Unsupported executable; inactive");
        return 0;
    }
    if (!gameSlots.discover(logMessage))
        return 0;
    MH_Initialize();
    ready = initCharacter();
    if (!ready) {
        logMessage("initialization failed");
        return 0;
    }
    logMessage("Game graphics factory observer installed=%d; no probe device",
               graphicsBootstrapInstalled);
    classInstall();
    aimCameraInstall();
    installVisibility();
    cutsceneVerified = GetPrivateProfileIntA("Framework", "CutscenePresentation", 1, ini) &&
                       PTDECutscene::verify();
    logMessage("CUTSCENE native layout signatures=%d", cutsceneVerified);
    if (nativeDamage)
        characterInstallDamage();
    worldInstall();
    defenseInstall();
    nativeRecoveryInstall();
    environmentFireInstall();
    projectileInstall();
    HostController::install(actionFilterPad, logMessage, feedbackInstallRumble);
    installActionInput();
    installActionEffects();
    installStartupHooks(nullptr);
    return 0;
}

static void installDeviceHooks(IDirect3DDevice9 *device) {
    if (WaitForSingleObject(startupComplete, 15000) != WAIT_OBJECT_0 || !ready) {
        logMessage("Graphics observer: host initialization unavailable");
        return;
    }
    static void *installedEntry = nullptr;
    void **table = *reinterpret_cast<void ***>(device);
    if (installedEntry == table[42])
        return;
    if (installedEntry) {
        logMessage("Graphics device implementation changed; refusing mixed trampolines");
        return;
    }
    void *targets[] = {table[42], table[16], table[17], table[37]};
    void *hooks[] = {reinterpret_cast<void *>(endScene), reinterpret_cast<void *>(reset),
                     reinterpret_cast<void *>(present), reinterpret_cast<void *>(setRenderTarget)};
    void **originals[] = {reinterpret_cast<void **>(&originalEnd),
                          reinterpret_cast<void **>(&originalReset),
                          reinterpret_cast<void **>(&originalPresent),
                          reinterpret_cast<void **>(&originalRenderTarget)};
    unsigned created = 0;
    for (; created < 4; ++created) {
        auto status = MH_CreateHook(targets[created], hooks[created], originals[created]);
        if (status != MH_OK)
            break;
    }
    bool enabled = created == 4;
    if (enabled)
        for (unsigned i = 0; i < 4; ++i)
            if (MH_EnableHook(targets[i]) != MH_OK) {
                enabled = false;
                break;
            }
    if (!enabled) {
        for (unsigned i = 0; i < created; ++i) {
            MH_DisableHook(targets[i]);
            MH_RemoveHook(targets[i]);
        }
        logMessage("Graphics device hook transaction failed; native rendering retained");
        return;
    }
    installedEntry = table[42];
    logMessage("Real game graphics device hooked=%p EndScene=%p", device, installedEntry);
}

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE h, DWORD v, REFIID r, LPVOID *out,
                                             LPUNKNOWN unk) {
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
    logMessage("DirectInput forward real=%08x version=%x hr=%08lx", (unsigned)(uintptr_t)real,
               (unsigned)v, hr);
    return hr;
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        selfModule = h;
        DisableThreadLibraryCalls(h);
        startupComplete = CreateEventA(nullptr, TRUE, FALSE, nullptr);
        if (startupComplete && read<uint32_t>(0x400080) == 0xfc293654)
            graphicsBootstrapInstalled =
                HostGraphics::install(GetModuleHandleA(nullptr), installDeviceHooks);
        HANDLE t = CreateThread(NULL, 0, start, NULL, 0, NULL);
        if (t)
            CloseHandle(t);
    }
    return TRUE;
}
