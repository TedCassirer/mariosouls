#pragma once
#include "../../core/equipment.h"
#include "equipment_props.h"
static NativeEquipment equipmentCurrent;
static NativeEquipmentItem equipmentSwingItem;
static SCEquipment::Swing equipmentSwing;
static SC_Attachments equipmentHands = {sizeof(SC_Attachments)};
static bool equipmentConfirmedGuard = false, equipmentBroken = false,
            equipmentPreviousBlade = false;
static Vec equipmentBladeA{}, equipmentBladeB{}, equipmentOldBladeA{}, equipmentOldBladeB{};

static bool equipmentCapable() {
    return adapter.profile && (adapter.profile->capabilities & SC_CAP_EQUIPMENT) && adapter.api &&
           adapter.api->equipment && adapter.api->attachments;
}

static bool equipmentPyromancy() {
    return equipmentCapable() && (adapter.profile->capabilities & SC_CAP_PYROMANCY);
}

static uint32_t equipmentCasterKind(const NativeEquipmentItem &item) {
    return equipmentPyromancy() && item.pyromancy ? uint32_t(SC_EQUIPMENT_PYROMANCY) : 0u;
}

static void equipmentReset() {
    SC_EquipmentPose appearance = {sizeof(appearance), equipmentCasterKind(equipmentCurrent.right),
                                   equipmentCasterKind(equipmentCurrent.left)};
    InterlockedExchange(&equipmentGuardInput, 0);
    equipmentSwing.reset();
    equipmentHands = {sizeof(SC_Attachments)};
    equipmentConfirmedGuard = false;
    equipmentPreviousBlade = false;
    equipmentCurrent = {};
    if (equipmentCapable() && characterSession.instance.id >= 0) {
        adapter.api->equipment(characterSession.instance.id, &appearance);
    }
}

static bool equipmentMelee() {
    return (equipmentCurrent.right.kind == SC_EQUIPMENT_SWORD ||
            (adapter.profile && (adapter.profile->capabilities & SC_CAP_MELEE_FAMILIES))) &&
           !equipmentBroken && equipmentCurrent.valid && equipmentCurrent.oneHanded &&
           equipmentCurrent.right.melee && equipmentProp(equipmentCurrent.right.model);
}

static bool equipmentShield() {
    return !equipmentBroken && equipmentCurrent.valid && equipmentCurrent.oneHanded &&
           equipmentCurrent.left.shield && equipmentProp(equipmentCurrent.left.model);
}

static void equipmentInput(SC_Input &input) {
    if (!equipmentCapable()) {
        InterlockedExchange(&equipmentGuardInput, 0);
        return;
    }
    auto next = equipmentRead();
    bool changed =
        next.valid != equipmentCurrent.valid || next.right.id != equipmentCurrent.right.id ||
        next.left.id != equipmentCurrent.left.id || next.stance != equipmentCurrent.stance;
    if (changed) {
        equipmentReset();
    }
    equipmentCurrent = next;
    bool melee = equipmentMelee(), shield = equipmentShield();
    // Native supported weapon attacks have their own routing and damage. Only
    // guest punches/kicks require the optional unarmed focus.
    if (!melee && !combatUnarmedReady())
        input.attack = 0;
    bool grounded = (characterSession.instance.state.flags & SC_GROUNDED) &&
                    (characterSession.instance.state.flags & SC_CAN_EQUIP_ACTION) &&
                    characterSession.instance.state.attackId < 0 && !pendingNativeHurt &&
                    !actionPending() && !characterLandingRecovery;
    bool eligible = melee && grounded &&
                    (equipmentSwing.running() || (!input.jump && !input.crouch)) && combatEnabled &&
                    hostPermits(SCHost::Operation::Combat);
    bool intent = equipmentSwing.tick(input.attack, eligible, equipmentCurrent.right.id);
    if (intent) {
        int combo = equipmentSwing.intendedCombo(),
            cost = equipmentCurrent.right.attacks[combo].stamina,
            stamina = read<int>(player + 0x2e4);
        if (cost > 0 && stamina >= cost) {
            equipmentSwingItem = equipmentCurrent.right;
            equipmentSwing.begin(equipmentSwingItem.id, equipmentSwingItem.kind);
            characterStaminaCeiling = stamina - cost;
            put<int>(player + 0x2e4, characterStaminaCeiling);
            characterRecoveryUntil = GetTickCount() + 500;
            equipmentPreviousBlade = false;
        }
    }
    if (melee || equipmentCasterKind(equipmentCurrent.right))
        input.attack = 0;
    bool guarding = shield && grounded && !equipmentSwing.running() && !input.jump &&
                    !input.attack && !input.crouch && read<int>(player + 0x2e4) > 0 && input.guard;
    InterlockedExchange(&equipmentGuardInput, guarding ? 1 : 0);
    equipmentConfirmedGuard = guarding && equipmentNativeGuard();
    if (equipmentSwing.running()) {
        // Windup/strike commitment cannot be turned into a running multi-hit sweep.
        input.moveX = input.moveY = 0;
        input.jump = input.crouch = 0;
        characterRecoveryUntil = GetTickCount() + 500;
        characterStaminaCeiling = std::min(characterStaminaCeiling, read<int>(player + 0x2e4));
    } else if (guarding) {
        input.moveX *= .35f;
        input.moveY *= .35f;
    }
    SC_EquipmentPose pose = {
        sizeof(pose),
        melee ? equipmentCurrent.right.kind : equipmentCasterKind(equipmentCurrent.right),
        shield ? uint32_t(SC_EQUIPMENT_SHIELD) : equipmentCasterKind(equipmentCurrent.left),
        equipmentSwing.running() ? uint32_t(equipmentSwing.combo + 1) : 0,
        equipmentSwing.phase(),
        equipmentConfirmedGuard ? 1.f : 0.f};
    if (!adapter.api->equipment(characterSession.instance.id, &pose)) {
        logMessage("Equipment presentation rejected; disabling equipment");
        equipmentBroken = true;
        equipmentReset();
    }
}

static bool equipmentGetHands() {
    equipmentHands = {sizeof(SC_Attachments)};
    if (!equipmentCapable() ||
        !adapter.api->attachments(characterSession.instance.id, &equipmentHands) ||
        equipmentHands.size != sizeof(equipmentHands) || (equipmentHands.valid & ~3u))
        return false;
    if ((equipmentHands.valid & 1) && !equipmentSocketValid(equipmentHands.right))
        equipmentHands.valid &= ~1u;
    if ((equipmentHands.valid & 2) && !equipmentSocketValid(equipmentHands.left))
        equipmentHands.valid &= ~2u;
    return equipmentHands.valid != 0;
}

static void equipmentContact() {
    if (!equipmentCapable() || !equipmentSwing.running() || equipmentBroken)
        return;
    auto now = equipmentRead();
    if (!now.valid || !now.oneHanded || now.right.id != equipmentSwingItem.id ||
        now.left.id != equipmentCurrent.left.id) {
        equipmentReset();
        return;
    }
    auto prop = equipmentProp(equipmentSwingItem.model);
    if (!prop || !equipmentGetHands() || !(equipmentHands.valid & 1)) {
        equipmentPreviousBlade = false;
        return;
    }
    bool headed = equipmentSwingItem.kind == SC_EQUIPMENT_AXE ||
                  equipmentSwingItem.kind == SC_EQUIPMENT_HAMMER;
    float radius = headed ? std::clamp(std::max(prop->bounds[3] - prop->bounds[0],
                                                prop->bounds[4] - prop->bounds[1]) *
                                           .5f,
                                       .09f, .22f)
                          : .09f;
    equipmentBladeA =
        equipmentPoint(equipmentHands.right, {0, 0, headed ? prop->bounds[5] * .55f : .02f},
                       characterSession.instance.origin);
    equipmentBladeB = equipmentPoint(equipmentHands.right, {0, 0, prop->bounds[5]},
                                     characterSession.instance.origin);
    Vec previousA = equipmentPreviousBlade ? equipmentOldBladeA : equipmentBladeA,
        previousB = equipmentPreviousBlade ? equipmentOldBladeB : equipmentBladeB;
    equipmentOldBladeA = equipmentBladeA;
    equipmentOldBladeB = equipmentBladeB;
    equipmentPreviousBlade = true;
    if (!equipmentSwing.active() || !combatEnabled || !hostPermits(SCHost::Operation::Combat))
        return;
    uintptr_t r = read<uint32_t>(gameSlots.player), begin = read<uint32_t>(r + 4),
              end = read<uint32_t>(r + 8);
    if (!begin || end < begin || end - begin > 16384)
        return;
    Vec feet = characterSession.instance.origin + Vec{characterSession.instance.state.position[0],
                                                      characterSession.instance.state.position[1],
                                                      characterSession.instance.state.position[2]} *
                                                      .01f;
    auto point = [](Vec v) { return SCContact::Point{v.x, v.y, v.z}; };
    for (uintptr_t at = begin; at < end; at += 4) {
        uintptr_t npc = read<uint32_t>(at);
        if (!npc || npc == player)
            continue;
        int team = read<int>(npc + 0x74), hp = read<int>(npc + 0x2d4);
        if ((team != 6 && team != 7 && team != 9) || hp <= 0 || hp > 100000)
            continue;
        uint32_t handle = read<uint32_t>(npc + 4);
        if (!nativeTarget(npc, handle) || equipmentSwing.targets.count(int(handle)))
            continue;
        uintptr_t np = chain(npc + 0x28, {0, 0x1c});
        if (!np)
            continue;
        Vec center = read<Vec>(np + 0x10);
        SCContact::Capsule cap;
        if (dot(center - feet, center - feet) > 400 || !targetCapsule(npc, center, cap))
            continue;
        SCContact::Point hit;
        if (!SCEquipment::sweep(point(previousA), point(previousB), point(equipmentBladeA),
                                point(equipmentBladeB), cap, radius, hit))
            continue;
        Vec contact = {hit.x, hit.y, hit.z};
        if (!lineClear(feet + Vec{0, .65f, 0}, contact) ||
            !worldLineClear(feet + Vec{0, .65f, 0}, contact, npc) ||
            !equipmentSwing.claim(int(handle)))
            continue;
        debugContact(contact, false);

        int result =
            equipmentNativeHit(equipmentSwingItem, equipmentSwing.combo, npc, handle, contact);

        if (result == -2)
            continue; // Area streaming must not permanently disable melee.
        if (!equipmentPacket.built || !equipmentPacket.valid) {
            equipmentBroken = true;
            equipmentReset();
            logMessage("Native equipment packet rejected; equipment disabled");
            return;
        }
        feedbackTrack(npc, int(handle), hp, 1000000 + equipmentSwing.serial, SC_MAX_ATTACKS,
                      contact);
    }
    // Native world-contact points are also eligible object hit locations. Do not
    // fabricate destructible bounds from an object's origin or visual mesh.
    if (worldObjectDamageReady)
        for (const auto &c : worldContacts) {
            if (c.kind != 1 || !c.owner)
                continue;
            uintptr_t object = c.owner;
            int handle = read<int>(object + 4), hp = read<int16_t>(object + 0x2a);
            if (hp <= 0 || !(read<uint8_t>(object + 0x2d) & 0x40) ||
                !read<uintptr_t>(object + 0x90) || equipmentSwing.targets.count(handle))
                continue;
            SCContact::Capsule sample = {
                {c.point.x, c.point.y - .05f, c.point.z}, .05f, .05f, .12f};
            SCContact::Point hit;
            if (!SCEquipment::sweep(point(previousA), point(previousB), point(equipmentBladeA),
                                    point(equipmentBladeB), sample, radius, hit) ||
                !lineClear(feet + Vec{0, .65f, 0}, c.point) || !equipmentSwing.claim(handle))
                continue;
            const auto &attack = equipmentSwingItem.attacks[equipmentSwing.combo];
            equipmentPacket = {};
            equipmentPacket.expected = true;
            equipmentPacket.weapon = equipmentSwingItem.id;
            equipmentPacket.behavior = attack.behavior;
            equipmentPacket.attack = attack.attack;
            alignas(16) float at[4] = {c.point.x, c.point.y, c.point.z, 1};
            worldDamageObject(attack.behavior, player, object, at);
            equipmentPacket.expected = false;
            if (!equipmentPacket.built || !equipmentPacket.valid) {
                equipmentBroken = true;
                equipmentReset();
                return;
            }
            feedbackTrack(object, handle, hp, 1000000 + equipmentSwing.serial, SC_MAX_ATTACKS,
                          c.point, true);
        }
}

static void equipmentDraw(SCRenderAPI::Renderer &renderer, Vec renderOrigin,
                          SCRenderAPI::Context context) {
    if (nativeActionOwned() || nativeReaction || equipmentBroken || !equipmentGetHands())
        return;
    if (equipmentMelee() && (equipmentHands.valid & 1))
        equipmentDrawProp(renderer, *equipmentProp(equipmentCurrent.right.model),
                          equipmentHands.right, renderOrigin, 0, context);
    if (equipmentShield() && (equipmentHands.valid & 2))
        equipmentDrawProp(renderer, *equipmentProp(equipmentCurrent.left.model),
                          equipmentHands.left, renderOrigin, 1, context);
}
