#pragma once
#include "../../core/launcher.h"
static const SC_LauncherAPI *launcherApi = nullptr;
static SC_LauncherState launcherState = {sizeof(SC_LauncherState)};
static Vec launcherPlace{}, launcherBuddy{};
static int launcherMap = -1, launcherTarget = 0;
static bool launcherUnlocked = false, launcherAvailable = false;
static bool launcherGrounded = false;
static float launcherYaw = 0;
static DWORD launcherMessageUntil = 0, launcherStarted = 0;
static std::vector<float> launcherPositions, launcherNormals, launcherColors, launcherUV;
static std::vector<uint8_t> launcherPixels;
static SC_Mesh launcherMesh{};
static IDirect3DTexture9 *launcherTexture = nullptr;
static char launcherSection[96] = {};

static bool launcherBusy() {
    return launcherState.phase == SC_LAUNCHER_OPENING || launcherState.phase == SC_LAUNCHER_AIMING;
}

static void launcherCameraRelease() {
    aimCameraSet(false);
}

static void launcherCameraTick() {
    static unsigned previousPhase = 0;
    if (launcherState.phase == SC_LAUNCHER_FLIGHT && previousPhase == SC_LAUNCHER_AIMING)
        logf("WORLD launcher fired yaw=%.5f pitch=%.5f local_cm=%.2f,%.2f,%.2f", launcherState.yaw,
             launcherState.pitch, launcherState.position[0], launcherState.position[1],
             launcherState.position[2]);
    previousPhase = launcherState.phase;
    if (launcherState.phase != SC_LAUNCHER_AIMING) {
        aimCameraSet(false);
        return;
    }
    float y = launcherState.yaw, p = launcherState.pitch;
    Vec at = origin +
             Vec{launcherState.position[0], launcherState.position[1], launcherState.position[2]} *
                 .01f +
             Vec{0, 1.25f, 0};
    aimCameraSet(true, at, {sinf(y) * cosf(p), sinf(p), cosf(y) * cosf(p)});
}

static void launcherCancel() {
    if (launcherApi && character >= 0)
        launcherApi->cancel(character);
    launcherState = {sizeof(SC_LauncherState)};
    launcherTarget = 0;
    launcherCameraRelease();
}

static void launcherConfigure() {
    auto entry = (SC_GetLauncherAPI)GetProcAddress(adapterModule, "SoulsCharacterGetLauncherAPI");
    launcherApi = entry ? entry(SC_LAUNCHER_VERSION, sizeof(SC_LauncherAPI)) : nullptr;
    if (!SCLauncher::valid(launcherApi)) {
        launcherApi = nullptr;
        return;
    }
    launcherPixels.resize(launcherApi->textureWidth * launcherApi->textureHeight * 4);
    SC_Texture t = {launcherPixels.data(), uint32_t(launcherPixels.size())};
    if (!launcherApi->texture(&t)) {
        launcherApi = nullptr;
        launcherPixels.clear();
        return;
    }
    launcherPositions.resize(launcherApi->maxTriangles * 9);
    launcherNormals.resize(launcherPositions.size());
    launcherColors.resize(launcherPositions.size());
    launcherUV.resize(launcherApi->maxTriangles * 6);
    launcherMesh = {launcherPositions.data(), launcherNormals.data(),    launcherColors.data(),
                    launcherUV.data(),        launcherApi->maxTriangles, 0};
    logf("WORLD launcher capability loaded: %s / %s", launcherApi->name, launcherApi->operatorName);
}

static float launcherSetting(const char *key, float fallback) {
    char value[40], def[40];
    snprintf(def, sizeof(def), "%.4f", fallback);
    GetPrivateProfileStringA(launcherSection, key, def, value, sizeof(value), ini);
    char *end = nullptr;
    float n = strtof(value, &end);
    return end && !*end && isfinite(n) && fabsf(n) < 1000 ? n : fallback;
}

static void launcherObserve(bool focus) {
    launcherTarget = 0;
    launcherAvailable = false;
    if (!launcherApi || !aimCameraReady || !characterOwned() || nativeActionOwned() ||
        characterSession.death()) {
        if (launcherState.phase)
            launcherCancel();
        return;
    }
    int map = currentMap();
    bool qa = map == 1801 && harnessIdentity() &&
              GetPrivateProfileIntA("WorldObjects", "QALauncher", 0, ini);
    if (map != 1002 && !qa) {
        if (launcherState.phase)
            launcherCancel();
        return;
    }
    if (map != launcherMap || !launcherGrounded) {
        launcherCancel();
        launcherMap = map;
        launcherGrounded = true;
        snprintf(launcherSection, sizeof(launcherSection), "WorldObjects.%s",
                 qa ? "QALauncher" : "FirelinkLauncher");
        launcherPlace = {launcherSetting("X", qa ? 3.1f : -45.f),
                         launcherSetting("Y", qa ? 196.5f : -60.5f),
                         launcherSetting("Z", qa ? 1.5f : 56.f)};
        launcherYaw = launcherSetting("Yaw", -1.5707963f);
        launcherBuddy = launcherPlace + Vec{2.5f, 0, 0};
        launcherUnlocked = GetPrivateProfileIntA(launcherSection, "Unlocked", 0, ini) != 0;
        // Snap only the placement, never the player, onto verified guest ground.
        Vec local = (launcherPlace - origin) * 100;
        float floor = guest->floorHeight(local.x, local.y + 200, local.z);
        if (isfinite(floor) && fabsf(floor - local.y) < 200)
            launcherPlace.y = origin.y + floor * .01f;
        else
            launcherGrounded = false;
        local = (launcherBuddy - origin) * 100;
        floor = guest->floorHeight(local.x, local.y + 200, local.z);
        if (isfinite(floor) && fabsf(floor - local.y) < 200)
            launcherBuddy.y = origin.y + floor * .01f;
        else
            launcherGrounded = false;
    }
    if (!launcherGrounded)
        return;
    launcherAvailable = true;
    if (!launcherApi->query(character, &launcherState) || !SCLauncher::valid(launcherState)) {
        launcherCancel();
        return;
    }
    if (launcherBusy() &&
        (!focus || nativeReaction || equipmentActionOwned || read<int>(player + 0x2d4) < lastHP)) {
        launcherCancel();
        return;
    }
    if (launcherBusy() && DWORD(GetTickCount() - launcherStarted) > 120000) {
        launcherCancel();
        return;
    }
    if (!focus || launcherState.phase || equipmentActionOwned || nativeReaction ||
        !(state.flags & SC_GROUNDED) || actionEligible(1))
        return;
    Vec feet = read<Vec>(posData + 0x10);
    if (SCLauncher::withinReach(&feet.x, &launcherBuddy.x, 1.6f, 1) &&
        lineClear(feet + Vec{0, .7f, 0}, launcherBuddy + Vec{0, .7f, 0}))
        launcherTarget = 1;
    else if (launcherUnlocked && SCLauncher::withinReach(&feet.x, &launcherPlace.x, 1.05f, 1) &&
             lineClear(feet + Vec{0, .7f, 0}, launcherPlace + Vec{0, .7f, 0}))
        launcherTarget = 2;
}

static bool launcherInteract() {
    if (!launcherTarget)
        return false;
    if (launcherTarget == 1) {
        launcherUnlocked = true;
        WritePrivateProfileStringA(launcherSection, "Unlocked", "1", ini);
        launcherMessageUntil = GetTickCount() + 3500;
        logf("WORLD operator unlocked launcher: %s", launcherSection);
        return true;
    }
    Vec local = (launcherPlace - origin) * 100;
    if (launcherApi->begin(character, &local.x, launcherYaw)) {
        launcherStarted = GetTickCount();
        launcherApi->query(character, &launcherState);
        characterInput.clear();
        combatReset();
        equipmentReset();
        logf("WORLD launcher entered");
    }
    return true;
}

// Filled by the host's verified camera hook. Never change the view used only by
// guest drawing: native terrain and character must use the same camera frame.
