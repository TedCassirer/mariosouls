#pragma once
#include "reaction_types.h"
#include "recovery_hooks.h"
// Native passive states (including guard break and damage reactions) retain
// ownership of motion. The guest supplies only its visible reaction pose.
static SCSession::QuietGate nativeReactionReturnGate;
static Vec nativeReactionPrevious;
static int nativeReactionMap = -1;
static SCReaction::Presentation nativeReactionPresentation;
static bool nativeReactionKnockdown = false;
static bool nativeReactionAirborne = false;

static int nativeActiveAllowed(uintptr_t actor = player, bool captured = true) {
    static int verified = 0;
    if (!verified) {
        // Chr ESD command100 / FDF1BB calls E2AEC0. Its bool assignment is at
        // E2AF34; E323B0 independently obtains the controller through movement+2c.
        const unsigned char setter[] = {0x88, 0x5e, 0x3c, 0x5e, 0x5b, 0xc2, 0x04, 0x00};
        const unsigned char access[] = {0x56, 0x8b, 0xf1, 0x8b, 0x46, 0x2c, 0x85, 0xc0};
        unsigned char setBytes[sizeof(setter)] = {}, accessBytes[sizeof(access)] = {};
        verified = get(uintptr_t(0xe2af34), setBytes) &&
                           !memcmp(setBytes, setter, sizeof(setter)) &&
                           get(uintptr_t(0xe323b0), accessBytes) &&
                           !memcmp(accessBytes, access, sizeof(access))
                       ? 1
                       : -1;
        logMessage("Native passive action predicate signature=%d", verified == 1);
    }
    if (verified != 1 || !actor || chain(gameSlots.player, {0, 4, 0}) != actor ||
        (captured && actor != ownedActor))
        return -1;
    uintptr_t movement = 0, action = 0, owner = 0;
    uint8_t allowed = 255;
    if (!get(actor + 0x28, movement) || !movement || (captured && movement != ownedMovement) ||
        !get(movement + 0x2c, action) || !action || !get(action + 0x10, owner) ||
        owner != movement || !get(action + 0x3c, allowed) || allowed > 1)
        return -1;
    return allowed;
}

static bool nativeReactionTick(double &acc, bool focus, bool priorityReaction = false) {
    nativeRecoveryReset();
    if (!nativeReaction && launcherBusy())
        return false;
    if (!nativeReaction && !defenseReady) {
        if (!equipmentCapable())
            return false;
        const auto current = equipmentRead();
        if (!current.valid || (!current.right.melee && !current.left.shield))
            return false;
    }
    int allowed = nativeActiveAllowed();
    if (allowed < 0) {
        logMessage("Native reaction predicate invalid; native handback");
        releaseControl();
        acc = 0;
        return true;
    }
    // Active-action ownership is also disabled by the native attack we inject.
    // It is not, on its own, evidence that an enemy hit or broke our guard.
    DWORD observed = GetTickCount();
    int observedHP = read<int>(player + 0x2d4);
    if (!nativeReaction) {
        if (observedHP > 0 && observedHP < lastHP) {
            if (equipmentConsumeNativeBlock(player, lastHP, observedHP)) {
                lastHP = observedHP;
                nativeReactionIncomingUntil = 0;
            } else
                nativeReactionIncomingUntil = observed + 200;
        }
        if (equipmentConsumeNativeGuardBreak(player))
            nativeReactionIncomingUntil = observed + 200;
        if (!SCCombat::pending(nativeReactionIncomingUntil, observed))
            return false;
    }
    if (!nativeReaction && allowed == 1)
        return false;
    // Classify only the verified passive reaction layer, never an active attack.
    uintptr_t tracks = chain(player + 0x28, {0, 0x18});
    uintptr_t track = tracks ? tracks + 9 * 0x9c : 0;
    uintptr_t categorized = tracks ? tracks + 25 * 0x9c : 0;
    int animation = tracks ? PTDEReaction::passiveAnimation(
                                 read<int>(track), read<uint32_t>(track + 4),
                                 read<int>(categorized), read<uint32_t>(categorized + 4))
                           : -1;
    auto reactionKind = PTDEReaction::classify(animation);
    PTDERecovery::State passive;
    const bool passiveValid = nativeRecoveryReady && PTDERecovery::inspect(player, passive);
    const bool ordinary =
        passiveValid && PTDERecovery::kind(passive.id) != PTDEReaction::Kind::Other;
    const bool airborneDamage = passiveValid && PTDERecovery::airborneDamage(passive.id);
    const bool airborneLanding = passiveValid && PTDERecovery::airborneLanding(passive.id);
    // The passive state is available before its animation reaches the track.
    // This avoids presenting a short flinch first for an incoming knockdown.
    if (ordinary)
        reactionKind = PTDERecovery::kind(passive.id);
    else if (airborneDamage)
        // The fall state does not identify hit strength. Keep a previously
        // classified knockdown; use the shorter flinch for a first observation.
        reactionKind = PTDEReaction::Kind::Flinch;
    // A paired throw can retain a passive track. Never accelerate its graph.
    bool paired = tracks && read<uint32_t>(tracks + 28 * 0x9c + 4) != 0xffffffffu &&
                  read<int>(tracks + 28 * 0x9c) >= 0;
    const bool priority = priorityReaction && !paired && (allowed == 1 || ordinary);
    // An injected attack also disables active actions. A recent HP decrease
    // must not turn that attack into a false stagger when poise held.
    if (!nativeReaction && !ordinary && !airborneDamage && defenseReady && animation >= 0 &&
        !(animation >= 2000 && animation <= 2100))
        return false;
    Vec host{};
    float nativeYaw = 0;
    int hp = 0, stamina = 0;
    if (!characterOwned() || nativeActionOwned() || character < 0 || !playerPhysics ||
        !playerCamera || !get(playerPhysics + 0x10, host) || !isfinite(host.x) ||
        !isfinite(host.y) || !isfinite(host.z) || !get(playerPhysics + 4, nativeYaw) ||
        !isfinite(nativeYaw) || !get(player + 0x2d4, hp) || hp <= 0 ||
        !get(player + 0x2e4, stamina) || stamina < -50 ||
        (nativeReaction &&
         (currentMap() != nativeReactionMap ||
          dot(host - nativeReactionPrevious, host - nativeReactionPrevious) > 64))) {
        logMessage("Native reaction identity/position invalid; native handback");
        releaseControl();
        acc = 0;
        return true;
    }
    float rate = PTDEReaction::rate(reactionKind, focus && allowed == 0 && !airborneDamage, paired,
                                    defenseProfile.lightReactionSpeed,
                                    defenseRecoveryReady ? defenseRecovery.knockdownSpeed : 1);
    defenseRateTick(rate);
    DWORD now = GetTickCount();
    if (!nativeReaction) {
        nativeReaction = true;
        nativeReactionPresentation.reset();
        nativeReactionKnockdown = reactionKind == PTDEReaction::Kind::Knockdown;
        nativeReactionAirborne = airborneDamage;
        nativeReactionMap = currentMap();
        nativeReactionReturnGate.clear();
        worldRelease();
        combatReset();
        actionReset();
        characterControl.resetSimulation();
        pendingNativeHurt = 0;

        // Emit the reaction pose immediately, then keep ordinary fixed-rate posing.
        acc = 1. / SC_FIXED_HZ;
    } else if (hp < lastHP) {
        // A second real hit starts a new one-shot reaction; elapsed render time
        // and native get-up transitions alone never restart the visible clip.
        nativeReactionPresentation.reset();
        nativeReactionKnockdown = reactionKind == PTDEReaction::Kind::Knockdown;
        nativeReactionAirborne = airborneDamage;
        nativeReactionReturnGate.clear();
    } else if (reactionKind == PTDEReaction::Kind::Knockdown && !nativeReactionKnockdown) {
        nativeReactionKnockdown = true;
        nativeReactionPresentation.reset();
    }
    if (airborneDamage)
        nativeReactionAirborne = true;
    nativeReactionPrevious = host;
    lastHP = hp;
    actionCancelPulse();
    InterlockedExchange(&equipmentGuardInput, 0);
    InterlockedExchange(&actionRoute, LONG(ActionRoute::Locked));
    actionFlags(true);
    Vec local = (host - origin) * 100;
    if (fabsf(local.x) > 4000 || fabsf(local.y) > 8000 || fabsf(local.z) > 4000) {
        origin = host;
        rebuildSurfaces();
        local = {0, 0, 0};
    }
    SC_Pose pose = {};
    pose.kind = SC_POSE_STAGGER;
    pose.phase = -1;
    if (defenseRecoveryReady && nativeReactionKnockdown)
        pose.kind = SC_POSE_KNOCKDOWN;
    pose.yaw = nativeYaw - 3.14159265f;
    pose.position[0] = local.x;
    pose.position[1] = local.y;
    pose.position[2] = local.z;
    for (int step = 0; acc >= 1. / SC_FIXED_HZ && step < 3; step++, acc -= 1. / SC_FIXED_HZ) {
        if (defenseRecoveryReady)
            pose.phase = nativeReactionPresentation.phase(
                nativeReactionKnockdown ? defenseRecovery.knockdownMs : defenseRecovery.flinchMs);
        if (priority) {
            // Keep the higher-priority guest action intact while the native
            // ordinary reaction executes its normal exit on the game thread.
            guest->setPosition(character, local.x, local.y, local.z);
        } else if (!guestPose(character, &pose, &state, &characterMesh.view)) {
            logMessage("Native reaction pose rejected; native handback");
            releaseControl();
            acc = 0;
            return true;
        }
        nativeReactionPresentation.advance(1.f / SC_FIXED_HZ, focus);
        pumpAudio();
    }
    const bool completed =
        defenseRecoveryReady &&
        nativeReactionPresentation.phase(nativeReactionKnockdown ? defenseRecovery.knockdownMs
                                                                 : defenseRecovery.flinchMs) >= 1;
    if ((completed || priority) && focus && !paired && passiveValid && allowed == 0 &&
        (PTDERecovery::kind(passive.id) != PTDEReaction::Kind::Other ||
         (nativeReactionAirborne && airborneLanding))) {
        AcquireSRWLockExclusive(&nativeRecoveryLock);
        nativeRecoveryRequest = {
            player, passive, hp, nativeReactionMap, now, nativeReactionAirborne && airborneLanding};
        ReleaseSRWLockExclusive(&nativeRecoveryLock);
    }
    // Animation completion requests the native state's normal exit. Control
    // resumes only after state0 has actually enabled active actions again.
    if (((completed || priority) && focus && allowed == 1) ||
        nativeReactionReturnGate.update(allowed == 1 && focus, now,
                                        defenseReady ? defenseProfile.returnQuietMs : 100)) {
        nativeRecoveryReset();
        if (priority)
            guest->setPosition(character, local.x, local.y, local.z);
        else
            guest->resume(character, local.x, local.y, local.z, pose.yaw);
        state.position[0] = local.x;
        state.position[1] = local.y;
        state.position[2] = local.z;
        state.velocity[0] = state.velocity[1] = state.velocity[2] = 0;
        state.forwardSpeed = 0;
        state.faceAngle = pose.yaw;
        SCState::clearCombat(state);
        pendingNativeHurt = 0;
        lastHP = hp;
        defenseRateRelease();
        nativeReaction = false;
        nativeReactionAirborne = false;
        nativeReactionIncomingUntil = 0;
        nativeReactionReturnGate.clear();
        worldActivate();
        characterControl.resetSimulation();
        actionFlags(false);
        InterlockedExchange(&actionRoute, LONG(ActionRoute::Character));
        acc = 0;
    }
    return true;
}
