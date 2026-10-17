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
        !get(playerPhysics + 0x10, host) || !isfinite(dot(host, host)) ||
        !get(playerPhysics + 4, yaw) || !isfinite(yaw) || !characterControl.session.die())
        return false;
    deathStarted = GetTickCount();
    deathPrevious = host;
    deathYaw = yaw - 3.14159265f;
    defenseRateRelease();
    worldRelease();
    bodyRelease();
    characterControl.resetSimulation();
    nativeReaction = false;
    nativeReactionIncomingUntil = 0;
    pendingNativeHurt = 0;
    combatReset();
    actionReset();

    guest->surfaces(nullptr, 0, 1);
    // Return native movement flags once. Continue hiding only the human model.
    auto f1 = read<uint32_t>(player + 0x1fc), f2 = read<uint32_t>(player + 0x3c4);
    put(player + 0x1fc, (f1 & ~0x4000u) | (savedFlags1 & 0x4000u));
    put(player + 0x3c4, actionControlLocks.update(f2, 0));
    if (disableMapHit || worldMode == 1)
        put(ownedMovement + 0xc4,
            (read<uint32_t>(ownedMovement + 0xc4) & ~0x10u) | (savedMapFlags & 0x10u));
    acc = 1. / SC_FIXED_HZ;

    return true;
}

static void deathTick(double &acc, int slot) {
    Vec host{};
    if (!get(playerPhysics + 0x10, host) || !isfinite(dot(host, host)) ||
        dot(host - deathPrevious, host - deathPrevious) > 64 ||
        DWORD(GetTickCount() - deathStarted) > 15000) {
        logMessage("Death presentation ended: native relocation or timeout");
        releaseControl(true);
        acc = 0;
        return;
    }
    deathPrevious = host;
    actionCancelPulse();
    InterlockedExchange(&equipmentGuardInput, 0);
    HostController::setOwner(slot);
    InterlockedExchange(&actionRoute, LONG(ActionRoute::Locked));
    Vec local = (host - origin) * 100;
    if (fabsf(local.x) > 4000 || fabsf(local.y) > 8000 || fabsf(local.z) > 4000) {
        origin = host;
        local = {};
    }
    SC_Pose pose = {SC_POSE_DEATH, {local.x, local.y, local.z}, deathYaw, -1, 0};
    for (int step = 0; acc >= 1. / SC_FIXED_HZ && step < 3; step++, acc -= 1. / SC_FIXED_HZ) {
        if (!guestPose(character, &pose, &state, &characterMesh.view)) {
            logMessage("Death pose rejected; native handback");
            releaseControl(true);
            acc = 0;
            return;
        }
        // Death poses must never expose an active attack to asynchronous consumers.
        SCState::clearCombat(state);
        pumpAudio();
    }
}
