#pragma once
#include "../../core/environment.h"
#include "native_environment.h"
static const SC_EnvironmentAPI *environmentAPI = nullptr;
static bool environmentReady = false;
static uint32_t environmentFrame = 0;
#include "fire_hooks.h"

static void environmentConfigure() {
    auto entry =
        (SC_GetEnvironmentAPI)GetProcAddress(adapter.module, "SoulsCharacterGetEnvironmentAPI");
    const auto api = entry ? entry(SC_ENVIRONMENT_VERSION, sizeof(SC_EnvironmentAPI)) : nullptr;
    environmentAPI = SCEnvironment::valid(api) ? api : nullptr;
    environmentReady = environmentAPI && PTDEEnvironment::available();
    logMessage("Native environment observations %s",
               environmentReady ? "available" : "unavailable; native damage unchanged");
}

static uint32_t environmentObserve(bool focus) {
    if (!environmentAPI || !characterOwned() || characterSession.instance.id < 0 ||
        player != characterSession.native.actor || read<int>(player + 0x2d4) <= 0)
        return 0;
    SC_Environment e{sizeof(e), SC_ENVIRONMENT_BURNING_FLOOR, 0, GetTickCount()};
    PTDEEnvironment::Effects effects;
    const bool valid = environmentReady && PTDEEnvironment::read(player, effects);
    uintptr_t stats = chain(0x1378700, {0, 8});
    int ring1 = -1, ring2 = -1;
    const bool rings = stats && get(stats + 0x280, ring1) && get(stats + 0x284, ring2);
    if (valid && rings && effects.lava)
        e.flags |= SC_ENV_CONTACT;
    if ((valid && effects.protectedLava) || (rings && (ring1 == 139 || ring2 == 139)))
        e.flags |= SC_ENV_PROTECTED;
    if ((characterSession.instance.state.flags & SC_GROUNDED) || worldNativeSupport.supported())
        e.flags |= SC_ENV_GROUNDED;
    auto native = nativeInteractionState();
    PTDERecovery::State passive;
    const bool ordinary = nativeRecoveryReady && PTDERecovery::inspect(player, passive) &&
                          PTDERecovery::kind(passive.id) != PTDEReaction::Kind::Other;
    const int allowed = nativeActiveAllowed();
    const uintptr_t tracks = read<uint32_t>(characterSession.native.movement + 0x18);
    const bool paired = !tracks || (read<uint32_t>(tracks + 28 * 0x9c + 4) != 0xffffffffu &&
                                    read<int>(tracks + 28 * 0x9c) >= 0);
    if (focus && native.valid && !native.menu && !native.event && !native.ladder &&
        !nativeActionOwned() && !equipmentActionOwned && !launcherBusy() && !paired) {
        if (allowed == 1 && !nativeReaction)
            e.flags |= SC_ENV_CONTROL_ALLOWED;
        if ((allowed == 1 || ordinary) && environmentFireReady && environmentConsumeFire()) {
            e.kind = SC_ENVIRONMENT_ENEMY_FIRE;
            e.flags |= SC_ENV_CONTACT | SC_ENV_CONTROL_ALLOWED;
        }
    }
    const uint32_t result = environmentAPI->observe(characterSession.instance.id, &e);
    if (result & SC_ENV_REACTION_STARTED) {
        combatReset();
    }
    return result;
}
