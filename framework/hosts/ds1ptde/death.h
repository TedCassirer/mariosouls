#pragma once
// Presentation only. The native dead actor keeps physics, HP, bloodstain,
// fade, loading and respawn ownership. No guest gameplay ticks run here.
static DWORD deathStarted = 0;
static Vec deathPrevious;
static float deathYaw = 0;

static bool deathBegin(double &acc) {
    Vec host{};
    float yaw = 0;
    if (!profile || !(profile->capabilities & SC_CAP_DEATH_POSE) || character < 0 ||
        !get(posData + 0x10, host) || !isfinite(dot(host, host)) || !get(posData + 4, yaw) ||
        !isfinite(yaw) || !characterControl.session.die())
        return false;
    deathStarted = GetTickCount();
    deathPrevious = host;
    deathYaw = yaw - 3.14159265f;
    worldRelease();
    bodyRelease();
    characterControl.clock.clear();
    characterControl.input.clear();
    nativeReaction = false;
    nativeReactionIncomingUntil = 0;
    pendingNativeHurt = 0;
    combatReset();
    actionReset();
    testFrames = testButtons = 0;
    guest->surfaces(nullptr, 0, 1);
    // Return native movement flags once. Continue hiding only the human model.
    auto f1 = read<uint32_t>(player + 0x1fc), f2 = read<uint32_t>(player + 0x3c4);
    put(player + 0x1fc, (f1 & ~0x4000u) | (savedFlags1 & 0x4000u));
    put(player + 0x3c4, (f2 & ~0x300u) | (savedFlags2 & 0x300u));
    if (disableMapHit || worldMode == 1)
        put(lastMap + 0xc4, (read<uint32_t>(lastMap + 0xc4) & ~0x10u) | (savedMapFlags & 0x10u));
    acc = 1. / SC_FIXED_HZ;
    logf("Death presentation begin actor=%08x hp=%d; native death owns physics and respawn",
         unsigned(player), read<int>(player + 0x2d4));
    return true;
}

static void deathTick(double &acc, int slot) {
    Vec host{};
    if (!get(posData + 0x10, host) || !isfinite(dot(host, host)) ||
        dot(host - deathPrevious, host - deathPrevious) > 64 ||
        DWORD(GetTickCount() - deathStarted) > 15000) {
        logf("Death presentation ended: native relocation or timeout");
        releaseControl(true);
        acc = 0;
        return;
    }
    deathPrevious = host;
    actionCancelPulse();
    InterlockedExchange(&equipmentGuardInput, 0);
    InterlockedExchange(&controllerOwnedSlot, slot);
    InterlockedExchange(&actionRoute, LONG(ActionRoute::Locked));
    if (hideNative)
        put(player + 0x258, 0.f);
    Vec local = (host - origin) * 100;
    if (fabsf(local.x) > 4000 || fabsf(local.y) > 8000 || fabsf(local.z) > 4000) {
        origin = host;
        local = {};
    }
    SC_Pose pose = {SC_POSE_DEATH, {local.x, local.y, local.z}, deathYaw, -1, 0};
    for (int step = 0; acc >= 1. / SC_FIXED_HZ && step < 3; step++, acc -= 1. / SC_FIXED_HZ) {
        if (!guestPose(character, &pose, &state, &characterMesh.view)) {
            logf("Death pose rejected; native handback");
            releaseControl(true);
            acc = 0;
            return;
        }
        // Death poses must never expose an active attack to asynchronous consumers.
        state.flags = 0;
        state.attackId = state.interruptedAttackId = -1;
        state.landingRecoveryTicks = 0;
        pumpAudio();
    }
}
