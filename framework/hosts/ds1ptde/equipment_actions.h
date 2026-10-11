#pragma once
#include "../../core/native_action.h"
// Native input requests only. No fabricated throw links, animation requests,
// damage packets, stamina writes or invulnerability flags in this lane.
static SCNativeAction::Buttons equipmentActionButtons;
static SCNativeAction::Completion equipmentActionCompletion;
static NativeEquipment equipmentActionGear;
static Vec equipmentActionPrevious;
static int equipmentActionMap = -1, equipmentActionAnimation = -1, equipmentActionCast = 0;

struct EquipmentActionTrack {
    int id = -1, kind = 0;
    float seconds = 0;
    bool busy = false;
};

static EquipmentActionTrack equipmentActionTrack() {
    EquipmentActionTrack out;
    uintptr_t tracks = chain(player + 0x28, {0, 0x18});
    if (!tracks)
        return out;
    for (int i = 0; i < 29; i++) {
        int id = -1;
        uint32_t token = 0;
        float seconds = 0;
        if (!get(tracks + i * 0x9c, id) || !get(tracks + i * 0x9c + 4, token) ||
            !get(tracks + i * 0x9c + 0x98, seconds))
            continue;
        if (token == 0xffffffffu || id < 0)
            continue;
        // Verified on the owned executable: parry layer22, paired throw layer28.
        int a = id % 10000, kind = 0;
        if (i == 28 && ((a >= 3200 && a < 3300) || (a >= 3400 && a < 3500)))
            kind = (a >= 3400 ? SC_EQUIPMENT_BACKSTAB : SC_EQUIPMENT_RIPOSTE);
        else if (i == 22 && a >= 5100 && a < 5200)
            kind = SC_EQUIPMENT_PARRY;
        else if (a >= 3000 && a < 3300)
            kind = SC_EQUIPMENT_LIGHT_1;
        else if (equipmentActionCast && (i == 6 || i == 17 || i == 22) && a >= 6200 && a < 6600)
            kind = equipmentActionCast;
        if (i != 14 && !HostInteraction::idleAnimation(id, token))
            out.busy = true;
        if (kind && isfinite(seconds) && seconds >= 0 && seconds < 30 &&
            (out.kind == 0 || i == 28)) {
            out.id = id;
            out.kind = kind;
            out.seconds = seconds;
        }
    }
    return out;
}

static bool equipmentCriticalCandidate(bool frontal) {
    uintptr_t r = read<uint32_t>(gameSlots.player), begin = read<uint32_t>(r + 4),
              end = read<uint32_t>(r + 8);
    if (!begin || end < begin || end - begin > 16384)
        return false;
    Vec feet = read<Vec>(posData + 0x10);
    float yaw = read<float>(posData + 4);
    Vec forward = {-sinf(yaw), 0, -cosf(yaw)};
    for (uintptr_t at = begin; at < end; at += 4) {
        uintptr_t npc = read<uint32_t>(at);
        if (!npc || npc == player)
            continue;
        int team = read<int>(npc + 0x74), hp = read<int>(npc + 0x2d4);
        if ((team != 6 && team != 7 && team != 9) || hp <= 0 || hp > 100000)
            continue;
        uintptr_t p = chain(npc + 0x28, {0, 0x1c});
        if (!p)
            continue;
        Vec target = read<Vec>(p + 0x10), delta = target - feet;
        float distance = sqrtf(delta.x * delta.x + delta.z * delta.z), facing = read<float>(p + 4);
        if (!isfinite(distance) || !isfinite(facing) || distance < .1f || distance > 1.7f ||
            fabsf(delta.y) > .65f)
            continue;
        if (dot(forward, delta) < distance * .6f)
            continue;
        Vec enemyForward = {-sinf(facing), 0, -cosf(facing)};
        // This is only an input-routing hint. Native throw eligibility remains final.
        uintptr_t tracks = chain(npc + 0x28, {0, 0x18});
        bool parried = tracks && read<uint32_t>(tracks + 25 * 0x9c + 4) != 0xffffffffu &&
                       read<int>(tracks + 25 * 0x9c) % 10000 == 2100;
        if (!frontal && !parried && dot(enemyForward, delta) < distance * .6f)
            continue;
        if (lineClear(feet + Vec{0, .6f, 0}, target + Vec{0, .6f, 0}) &&
            worldLineClear(feet + Vec{0, .6f, 0}, target + Vec{0, .6f, 0}, npc))
            return true;
    }
    return false;
}

static bool equipmentActionsInput(SC_Input &input, bool parry, bool focus, int slot) {
    auto edge = equipmentActionButtons.sample(input.attack, parry, focus, player, slot);
    if (equipmentActionButtons.suppressAttack())
        input.attack = 0;
    if (!focus || !characterOwned() || nativeActionOwned() || nativeReaction ||
        equipmentActionOwned || !profile || !(profile->capabilities & SC_CAP_NATIVE_COMBAT) ||
        !equipmentCapable() || equipmentBroken || equipmentSwing.running() || pendingNativeHurt ||
        actionPending() || characterLandingRecovery || !(state.flags & SC_GROUNDED) ||
        !(state.flags & SC_CAN_EQUIP_ACTION) || state.attackId >= 0 || input.jump || input.crouch ||
        !combatEnabled || read<int>(player + 0x2e4) <= 0)
        return false;
    auto gear = equipmentRead();
    auto o = nativeInteractionState();
    if (!gear.valid || !gear.oneHanded || !o.valid || !o.idle || o.menu || o.event || o.ladder)
        return false;
    int cast = equipmentPyromancy() ? (edge.attack && gear.right.pyromancy ? SC_EQUIPMENT_CAST_RIGHT
                                       : edge.parry && gear.left.pyromancy ? SC_EQUIPMENT_CAST_LEFT
                                                                           : 0)
                                    : 0;
    bool doParry = !cast && edge.parry && gear.left.shield && equipmentProp(gear.left.model);
    bool frontal = input.guard;
    bool doCritical = edge.attack &&
                      (gear.right.kind == SC_EQUIPMENT_SWORD ||
                       (profile->capabilities & SC_CAP_MELEE_FAMILIES)) &&
                      gear.right.melee && equipmentProp(gear.right.model) &&
                      equipmentCriticalCandidate(frontal);
    if (!cast && !doParry && !doCritical)
        return false;
    int pulse = cast == SC_EQUIPMENT_CAST_LEFT ? 5 : doParry ? 3 : 4;
    // Validate the current keyboard binding before handing off when no pad exists.
    if (slot < 0 && !actionCombatBindingReady(pulse))
        return false;
    combatReset();
    actionReset();
    worldRelease();
    characterControl.clock.clear();
    characterControl.input.clear();
    equipmentActionOwned = true;
    equipmentActionCast = cast;
    equipmentActionGear = gear;
    equipmentCurrent = gear;
    equipmentActionMap = currentMap();
    equipmentActionPrevious = read<Vec>(posData + 0x10);
    equipmentActionAnimation = -1;
    equipmentActionCompletion.begin(GetTickCount());
    equipmentActionButtons.consume();
    testFrames = testButtons = 0;
    pendingNativeHurt = 0;
    lastHP = read<int>(player + 0x2d4);
    InterlockedExchange(&actionRoute, LONG(ActionRoute::Locked));
    actionFlags(true);
    actionQueue(pulse, GetTickCount(), slot >= 0);
    logf("Native equipment request=%s right=%d left=%d",
         cast      ? "cast"
         : doParry ? "parry"
                   : "critical",
         gear.right.id, gear.left.id);
    return true;
}

static bool equipmentActionsTick(double &acc, bool focus) {
    if (!equipmentActionOwned)
        return false;
    Vec host = read<Vec>(posData + 0x10);
    auto o = nativeInteractionState();
    auto track = equipmentActionTrack();
    int castAnimation = track.id % 10000;
    projectileCastRate(track.kind >= SC_EQUIPMENT_CAST_RIGHT &&
                       (castAnimation == 6204 || castAnimation == 6304 || castAnimation == 6404 ||
                        castAnimation == 6504));
    auto gear = equipmentRead();
    if (!o.valid || !gear.valid || player != lastPlayer || currentMap() != equipmentActionMap ||
        !isfinite(host.y) ||
        dot(host - equipmentActionPrevious, host - equipmentActionPrevious) > 64 ||
        read<int>(player + 0x2d4) <= 0) {
        logf("Native equipment ownership invalid; native handback");
        releaseControl(true);
        acc = 0;
        return true;
    }
    if (!focus)
        actionCancelPulse();
    if (o.menu || o.event || o.ladder) {
        projectileRateRelease();
        equipmentActionOwned = false;
        beginInteraction(-1, false);
        return true;
    }
    // Never continue displaying an old weapon after a native gear change.
    equipmentActionGear = gear;
    equipmentCurrent = gear;
    equipmentActionPrevious = host;
    InterlockedExchange(&actionRoute, LONG(ActionRoute::Locked));
    actionFlags(true);
    if (hideNative) {
        put(player + 0x258, 0.f);
        put(player + 0x1fc, read<uint32_t>(player + 0x1fc) & ~0x800000u);
    }
    if (track.id != equipmentActionAnimation) {
        equipmentActionAnimation = track.id;
        logf("Native equipment animation=%d kind=%d hp=%d stamina=%d", track.id, track.kind,
             read<int>(player + 0x2d4), read<int>(player + 0x2e4));
    }
    Vec local = (host - origin) * 100;
    SC_Pose pose = {};
    pose.kind = SC_POSE_NATIVE_COMBAT;
    pose.phase = -1;
    pose.position[0] = local.x;
    pose.position[1] = local.y;
    pose.position[2] = local.z;
    pose.yaw = read<float>(posData + 4) - 3.14159265f;
    // Native Fireball: right windup/release 6304/6504, left 6204/6404.
    int releaseBase = track.kind == SC_EQUIPMENT_CAST_LEFT ? 6400 : 6500;
    float phase =
        track.kind >= SC_EQUIPMENT_CAST_RIGHT
            ? (track.id % 10000 < releaseBase ? std::min(.45f, track.seconds / .95f * .45f)
                                              : .45f + track.seconds / 1.85f * .55f)
        : track.kind == SC_EQUIPMENT_PARRY ? track.seconds / 1.1f
        : track.kind >= SC_EQUIPMENT_BACKSTAB
            ? track.seconds / 3.7f
            : track.seconds / (SCEquipment::timing(gear.right.kind).total / 30.f);
    SC_EquipmentPose e = {sizeof(e),
                          gear.right.melee ? gear.right.kind : equipmentCasterKind(gear.right),
                          gear.left.shield ? uint32_t(SC_EQUIPMENT_SHIELD)
                                           : equipmentCasterKind(gear.left),
                          uint32_t(track.kind),
                          std::clamp(phase, 0.f, 1.f),
                          0};
    if (track.kind == 0 && track.busy) {
        pose.kind = SC_POSE_STAGGER;
        e.swing = 0;
    }
    for (int step = 0; acc >= 1. / SC_FIXED_HZ && step < 3; step++, acc -= 1. / SC_FIXED_HZ) {
        if (!guest->equipment(character, &e) ||
            !guestPose(character, &pose, &state, &characterMesh.view)) {
            releaseControl(true);
            acc = 0;
            return true;
        }
        pumpAudio();
    }
    lastHP = read<int>(player + 0x2d4);
    if (equipmentActionCompletion.update(track.busy, o.idle, focus, GetTickCount())) {
        projectileRateRelease();
        actionCancelPulse();
        equipmentActionOwned = false;
        equipmentReset();
        guest->resume(character, local.x, local.y, local.z, pose.yaw);
        state.position[0] = local.x;
        state.position[1] = local.y;
        state.position[2] = local.z;
        state.faceAngle = pose.yaw;
        state.flags = 0;
        state.attackId = state.interruptedAttackId = -1;
        state.velocity[0] = state.velocity[1] = state.velocity[2] = state.forwardSpeed = 0;
        state.landingRecoveryTicks = 0;
        pendingNativeHurt = 0;
        nativeReactionIncomingUntil = 0;
        worldActivate();
        characterControl.clock.clear();
        characterControl.input.clear();
        actionFlags(false);
        InterlockedExchange(&actionRoute, LONG(ActionRoute::Character));
        acc = 0;
        logf("Native equipment complete; same character=%d resumes", character);
    }
    return true;
}
