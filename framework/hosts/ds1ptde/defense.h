#pragma once
#include "../../core/reaction.h"
static SC_ReactionProfile defenseProfile{};
static bool defenseSupported = false, defenseReady = false;
static SC_RecoveryProfile defenseRecovery{};
static bool defenseRecoveryReady = false;
extern "C" {
__attribute__((used)) void *defenseOriginalRegen = nullptr, *defenseOriginalHit = nullptr;

__attribute__((used, noinline)) void __cdecl characterPoiseAdjust(float *value, uintptr_t actor) {
    if (defenseReady && characterOwned() && actor == ownedActor && actor == player &&
        chain(gameSlots.player, {0, 4, 0}) == actor &&
        read<uintptr_t>(actor + 0x28) == ownedMovement && read<int>(actor + 0x2d4) > 0)
        *value = SCReaction::poise(*value, defenseProfile.bonusPoise);
}
}
#include "poise_hooks.h"

static void defenseConfigure() {
    auto entry =
        (SC_GetReactionProfile)GetProcAddress(adapterModule, "SoulsCharacterGetReactionProfile");
    const auto p = entry ? entry(SC_REACTION_PROFILE_VERSION, sizeof(SC_ReactionProfile)) : nullptr;
    defenseSupported = SCReaction::valid(p);
    if (!defenseSupported)
        return;
    defenseProfile = *p;
    std::string section = std::string("Defense.") + profile->id;
    defenseProfile.bonusPoise =
        float(GetPrivateProfileIntA(section.c_str(), "BonusPoise", int(p->bonusPoise), ini));
    defenseProfile.lightReactionSpeed =
        GetPrivateProfileIntA(section.c_str(), "LightReactionSpeedPercent",
                              int(p->lightReactionSpeed * 100), ini) /
        100.f;
    defenseProfile.returnQuietMs =
        GetPrivateProfileIntA(section.c_str(), "ReturnQuietMs", p->returnQuietMs, ini);
    if (!SCReaction::valid(&defenseProfile))
        defenseProfile = *p;
    auto recoveryEntry =
        (SC_GetRecoveryProfile)GetProcAddress(adapterModule, "SoulsCharacterGetRecoveryProfile");
    const auto recovery =
        recoveryEntry ? recoveryEntry(SC_RECOVERY_PROFILE_VERSION, sizeof(SC_RecoveryProfile))
                      : nullptr;
    defenseRecoveryReady = SCReaction::validRecovery(recovery);
    if (defenseRecoveryReady)
        defenseRecovery = *recovery;
}

static bool defenseInstall() {
    if (!defenseSupported)
        return false;
    // Owned PTDE executable: computed native poise cap, after x87 FSTP.
    // Both regeneration and incoming-damage clamp must use the same budget.
    const unsigned char regen[] = {0xf3, 0x0f, 0x10, 0x14, 0x24};
    const unsigned char hit[] = {0xf3, 0x0f, 0x10, 0x4c, 0x24, 0x1c};
    unsigned char a[sizeof(regen)]{}, b[sizeof(hit)]{};
    if (!get(uintptr_t(0xe67f0a), a) || memcmp(a, regen, sizeof(a)) ||
        !get(uintptr_t(0xe6869e), b) || memcmp(b, hit, sizeof(b)))
        return false;
    void *atA = (void *)0xe67f0a, *atB = (void *)0xe6869e;
    if (MH_CreateHook(atA, (void *)defenseRegenHook, &defenseOriginalRegen) != MH_OK)
        return false;
    if (MH_CreateHook(atB, (void *)defenseHitHook, &defenseOriginalHit) != MH_OK ||
        MH_EnableHook(atA) != MH_OK || MH_EnableHook(atB) != MH_OK) {
        MH_DisableHook(atA);
        MH_DisableHook(atB);
        MH_RemoveHook(atA);
        MH_RemoveHook(atB);
        return false;
    }
    defenseReady = true;
    logMessage("Native defense enabled bonus_poise=%.1f light_rate=%.2f quiet_ms=%u",
               defenseProfile.bonusPoise, defenseProfile.lightReactionSpeed,
               defenseProfile.returnQuietMs);
    return true;
}

static SCProjectile::Rate defenseRate;
static uintptr_t defenseRateActor = 0, defenseRateAddress = 0;

static void defenseRateRelease() {
    float current = 0, value = 0;
    if (defenseRateAddress && chain(gameSlots.player, {0, 4, 0}) == defenseRateActor &&
        chain(defenseRateActor + 0x28, {0, 0x14}) + 0x64 == defenseRateAddress &&
        get(defenseRateAddress, current) && defenseRate.restore(current, value))
        put(defenseRateAddress, value);
    defenseRate = {};
    defenseRateActor = defenseRateAddress = 0;
}

static void defenseRateTick(float scale) {
    if (!defenseReady || scale <= 1) {
        defenseRateRelease();
        return;
    }
    uintptr_t graph = chain(player + 0x28, {0, 0x14});
    if (!graph) {
        defenseRateRelease();
        return;
    }
    uintptr_t at = graph + 0x64;
    if (defenseRateAddress && (defenseRateAddress != at || defenseRateActor != player))
        defenseRateRelease();
    float current = 0, next = 0;
    if (get(at, current) && defenseRate.acquire(current, scale, next) && put(at, next)) {
        defenseRate.last = next;
        defenseRateActor = player;
        defenseRateAddress = at;
    }
}
