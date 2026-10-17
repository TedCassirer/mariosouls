#pragma once
#include "target_geometry.h"
static SCCombat::Settings characterBalance;
static SCCombat::Window characterWindow;
static DWORD characterRecoveryUntil = 0;
static int characterStaminaCeiling = 0, characterLandingRecovery = 0;
static bool characterFalling = false;
static SCUnarmed::Gate characterUnarmedGate;

static bool combatUnarmedReady() {
    if (!unarmedEquipmentEnabled)
        return true;
    auto gear = equipmentRead();
    return characterUnarmedGate.observe(unarmedEquipmentLevel(gear) >= 0,
                                        gear.valid ? gear.right.id : -1,
                                        characterSession.instance.state.attackId);
}

static float characterPeakY = 0;
static constexpr int characterRecoveryDelay = 500;

static int combatNativeStamina(uintptr_t actor, int value, uintptr_t caller, uintptr_t packet) {
    if (actor != player)
        return value;
    if (!characterOwned() || nativeActionOwned() || actor != characterSession.native.actor) {
        return value;
    }
    int current = 0;
    if (!get(actor + 0x2e4, current)) {
        return value;
    }
    equipmentObserveNativeBlock(actor, current, value, caller, packet);
    // The native character-state command1001 (FE024A ->46D550 ->E67960)
    // debits20 after the character's injected hit. Character owns attack/movement costs
    // while active. Incoming hit/block drain and effect routines still pass.
    if (caller == 0xfe024f && value < current) {
        return current;
    }
    // Delay ordinary regeneration before it reaches the shared native pool.
    if (caller == 0xe80c13 && value > current &&
        SCCombat::pending(characterRecoveryUntil, GetTickCount())) {
        return std::min(current, characterStaminaCeiling);
    }
    return value;
}

static std::unordered_map<int, DWORD> characterLastStagger;
static std::unordered_map<int, SCImpact::Cooldown> characterLastPush;
static char combatConfigSection[96] = {};

static float combatFloat(const char *key, float fallback, float lo, float hi) {
    char d[40], v[40];
    snprintf(d, sizeof(d), "%.4f", fallback);
    GetPrivateProfileStringA(combatConfigSection, key, d, v, sizeof(v), ini);
    float f = strtof(v, nullptr);
    return isfinite(f) ? std::clamp(f, lo, hi) : fallback;
}

static void combatSettings() {
    feedbackSettings();
    snprintf(combatConfigSection, sizeof(combatConfigSection), "Combat.%s", adapter.profile->id);
    characterBalance.base = combatFloat("BasePower", adapter.profile->basePower, 1, 1000);
    characterBalance.upgradeStep =
        combatFloat("UpgradePower", adapter.profile->upgradePower, 0, 50);
    characterBalance.dexterityWeight = adapter.profile->dexterityWeight;
    characterBalance.fixedPower = combatFloat("FixedPower", 0, 0, 1200);
    characterBalance.upgrade =
        std::clamp<int>(GetPrivateProfileIntA(combatConfigSection, "UpgradeLevel", 0, ini), 0, 15);
    characterBalance.scaling =
        GetPrivateProfileIntA(combatConfigSection, "StatScaling", 1, ini) != 0;

    for (unsigned i = 0; i < adapter.profile->attackCount; i++) {
        char key[80];
        snprintf(key, sizeof(key), "%s_multiplier", attackName(i));
        characterBalance.multiplier[i] =
            combatFloat(key, adapter.profile->attacks[i].multiplier, 0, 3);
        snprintf(key, sizeof(key), "%s_cost", attackName(i));
        characterBalance.poise[i] = adapter.profile->attacks[i].poise;
        characterBalance.cost[i] =
            std::clamp<int>(GetPrivateProfileIntA(combatConfigSection, key,
                                                  adapter.profile->attacks[i].staminaCost, ini),
                            0, 100);
    }
}

static void combatReset() {
    characterUnarmedGate.reset();
    equipmentClearNativeBlock();
    equipmentReset();
    feedbackReset();
    characterWindow.reset();
    characterRecoveryUntil = 0;
    characterStaminaCeiling = 0;
    characterLandingRecovery = 0;
    characterLastStagger.clear();
    characterLastPush.clear();
    characterFalling = false;
}

static bool environmentKill(double &acc, const char *reason) {
    if (!characterOwned() || nativeActionOwned() || player != characterSession.native.actor ||
        chain(gameSlots.player, {0, 4, 0}) != player)
        return false;
    int hp = read<int>(player + 0x2d4);
    if (hp <= 0)
        return false;
    if (!put<int>(player + 0x2d4, 0))
        return false;

    launcherCancel();
    if (!deathBegin(acc))
        releaseControl(true);
    return true;
}

static bool combatEnvironment(double &acc) {
    float y =
        characterSession.instance.origin.y + characterSession.instance.state.position[1] * .01f;
    // Native support is resolved before the next guest step. In particular a
    // descending lift can transport the actor while the previous guest frame
    // still says airborne. Count actual unsupported descent, not lift travel.
    bool grounded =
        bool(characterSession.instance.state.flags & SC_GROUNDED) || worldNativeSupport.supported();
    if (!grounded) {
        if (!characterFalling) {
            characterPeakY = y;
            characterFalling = true;
        }
        characterPeakY = std::max(characterPeakY, y);
    }
    if (!characterFalling)
        return true;
    float drop = characterPeakY - y;
    int maxHP = read<int>(player + 0x2d8), hp = read<int>(player + 0x2d4);
    float safe = combatFloat("SafeFallMeters", 8, 6, 20),
          lethal = combatFloat("LethalFallMeters", 20, safe + 2, 50);
    if (grounded || (!characterBody.nonlethalFalls && drop >= lethal)) {
        int base = SCCombat::fallDamage(drop, maxHP, safe, lethal);
        int damage = SCBody::landingDamage(base, hp, maxHP, characterBody);
        characterFalling = false;
        // Only while Character owns gravity. Native interactions/falling use native logic.
        if (damage > 0 && hp > 0 && maxHP > 0 && maxHP < 100000 &&
            (read<uint32_t>(player + 0x1fc) & 0x4000) && player == characterSession.native.actor) {
            int after = std::max(0, hp - damage);
            put<int>(player + 0x2d4, after);
            if (after == 0) {
                if (!deathBegin(acc))
                    releaseControl(true);
                return false;
            }
        }
    }
    return true;
}

static void combatInput(SC_Input &in) {
    if (!hostPermits(SCHost::Operation::Combat)) {
        in.attack = 0;
        characterWindow.reset();
        return;
    }
    int stamina = read<int>(player + 0x2e4);
    if (SCCombat::pending(characterRecoveryUntil, GetTickCount())) {
        characterStaminaCeiling = std::min(characterStaminaCeiling, stamina);
        if (stamina > characterStaminaCeiling)
            put<int>(player + 0x2e4, characterStaminaCeiling);
    }
    if (characterLandingRecovery > 0) {
        in.jump = in.attack = in.crouch = 0;
        in.moveX = in.moveY = 0;
        --characterLandingRecovery;
    }
}

static void combatPay() {
    int move = characterWindow.move;
    if (move < 0)
        return;
    uintptr_t stats = chain(0x1378700, {0, 8});
    int str = read<int>(stats + 0x50), dex = read<int>(stats + 0x58);
    int stamina = read<int>(player + 0x2e4), cost = characterBalance.cost[move];
    auto balance = characterBalance;
    if (unarmedEquipmentEnabled) {
        int level = unarmedEquipmentLevel(equipmentRead());
        if (level < 0)
            return;
        balance.upgrade = level;
    }
    float power = SCCombat::power(balance, str, dex) * balance.multiplier[move];
    bool paid = characterWindow.pay(stamina, cost, power);
    if (paid) {
        characterStaminaCeiling = stamina - cost;
        put<int>(player + 0x2e4, characterStaminaCeiling);
        characterRecoveryUntil = GetTickCount() + characterRecoveryDelay;
    }
}

static void combat() {
    if (!combatUnarmedReady()) {
        characterWindow.reset();
        characterLandingRecovery = 0;
        return;
    }
    if (!combatEnabled || !hostPermits(SCHost::Operation::Combat))
        return;
    bool grounded = bool(characterSession.instance.state.flags & SC_GROUNDED);
    int interrupted = characterSession.instance.state.interruptedAttackId;
    if (interrupted != SCCombat::None && interrupted != characterWindow.move &&
        interrupted != characterSession.instance.state.attackId) {
        if (characterWindow.observe(interrupted, false, adapter.profile->automaticAttackId))
            combatPay();
    }
    bool start = characterWindow.observe(characterSession.instance.state.attackId, grounded,
                                         adapter.profile->automaticAttackId);
    if (start)
        combatPay();
    if (characterSession.instance.state.landingRecoveryTicks)
        characterLandingRecovery = int(characterSession.instance.state.landingRecoveryTicks);
    if (characterWindow.move != SCCombat::None &&
        characterWindow.move != adapter.profile->automaticAttackId && !characterWindow.exhausted &&
        characterWindow.age < 24) {
        characterRecoveryUntil = GetTickCount() + characterRecoveryDelay;
        characterStaminaCeiling = std::min(characterStaminaCeiling, read<int>(player + 0x2e4));
    }
    uintptr_t root = read<uint32_t>(gameSlots.player), begin = read<uint32_t>(root + 4),
              end = read<uint32_t>(root + 8);
    if (!begin || end < begin || end - begin > 16384)
        return;
    Vec p = characterSession.instance.origin + Vec{characterSession.instance.state.position[0],
                                                   characterSession.instance.state.position[1],
                                                   characterSession.instance.state.position[2]} *
                                                   .01f;
    const auto *attack = attackDefinition(characterWindow.move);
    const auto *automatic = attackDefinition(adapter.profile->automaticAttackId);
    for (uintptr_t at = begin; at < end; at += 4) {
        uintptr_t npc = read<uint32_t>(at);
        if (!npc || npc == player)
            continue;
        int team = read<int>(npc + 0x74), hp = read<int>(npc + 0x2d4);
        if ((team != 6 && team != 7 && team != 9) || hp <= 0 || hp > 100000)
            continue;
        uint32_t handle = read<uint32_t>(npc + 4);
        if (!nativeTarget(npc, handle))
            continue;
        uintptr_t np = chain(npc + 0x28, {0, 0x1c});
        if (!np)
            continue;
        Vec center = read<Vec>(np + 0x10);
        bool downward = characterWindow.move == SCCombat::None ||
                        characterWindow.move == adapter.profile->automaticAttackId;
        SCContact::Capsule capsule;
        bool shaped = targetCapsule(npc, center, capsule);
        Vec pos = shaped ? targetContact(capsule, p, downward)
                         : center + Vec{0, downward ? 1.f : .7f, 0},
            delta = pos - p;
        float range = attack ? attack->rangeCm * .01f : automatic ? automatic->rangeCm * .01f : 0;
        if (!SCContact::inReach({p.x, p.y, p.z}, {pos.x, pos.y, pos.z}, range,
                                (attack ? attack->minHeightCm : -200) * .01f,
                                (attack ? attack->maxHeightCm : 140) * .01f))
            continue;
        if (!lineClear(p + Vec{0, .7f, 0}, pos) || !worldLineClear(p + Vec{0, .7f, 0}, pos, npc))
            continue;
        Vec local = (pos - characterSession.instance.origin) * 100;
        uint32_t contact =
            adapter.api->probeContact(characterSession.instance.id, local.x, local.y, local.z);
        // Invulnerable/knockback actions are never offensive stomp states. Require
        // feet above the enemy and tight horizontal overlap, not the old broad sphere.
        if (characterWindow.move == SCCombat::None && automatic &&
            (contact & SC_CONTACT_AUTOMATIC) &&
            (characterSession.instance.state.flags & SC_AUTO_ATTACK_ELIGIBLE) &&
            automaticContact(delta, *automatic)) {
            if (characterWindow.beginAutomatic(adapter.profile->automaticAttackId))
                combatPay();
        }
        int move = characterWindow.move;
        if (!characterWindow.active(characterSession.instance.state, *adapter.profile) ||
            !characterWindow.canHit(int(handle), *adapter.profile))
            continue;
        if (move == adapter.profile->automaticAttackId) {
            if (!automatic || !(contact & SC_CONTACT_AUTOMATIC) ||
                !automaticContact(delta, *automatic))
                continue;
        } else if (!(contact & SC_CONTACT_ATTACK))
            continue;
        if (read<uint32_t>(read<uint32_t>(0x137d644) + 0x3c) != player)
            continue;
        bool first = characterWindow.hit.empty();
        characterWindow.hit.insert(int(handle));
        debugContact(pos, false);
        DWORD now = GetTickCount();
        float poise = characterBalance.poise[move], push = -1;
        bool pushAttack = characterImpactSupported && characterImpact.knockbackCm[move] > 0;
        if (pushAttack) {
            auto &cooldown = characterLastPush[int(handle)];
            bool ready = cooldown.ready(npc, now, characterImpact.cooldownMs);
            float current = NAN, maximum = NAN;
            get(npc + 0x1c0, current);
            get(npc + 0x1c4, maximum);
            if (!ready)
                poise = 0;
            push = shaped && ready
                       ? SCImpact::distance(characterImpact.knockbackCm[move], current, maximum,
                                            poise, characterImpact.resistantPoise, capsule.radius,
                                            capsule.top + capsule.radius)
                       : 0;
            if (push > 0)
                cooldown.commit(npc, now);
        } else {
            auto last = characterLastStagger.find(int(handle));
            if (last != characterLastStagger.end() && DWORD(now - last->second) < 1000)
                poise = 0;
            else
                characterLastStagger[int(handle)] = now;
        }

        int result =
            characterNativeHit(move, characterWindow.attackPower, poise, npc, handle, pos, push);

        if (result == -2)
            continue; // Streamed/recycled target is not a broken damage hook.
        if (!characterPacketBuilt) {
            logMessage("Native damage pipeline unavailable");
            return;
        }
        feedbackTrack(npc, int(handle), hp, characterWindow.serial, move,
                      pos + unit(p + Vec{0, .7f, 0} - pos) * .05f);
        // Commit the guest contact reaction once, after eligibility/stamina checks. The
        // read-only probe never bounces an exhausted Character or resets an attack window.
        if (first && characterPacketBuilt)
            adapter.api->commitContact(characterSession.instance.id, local.x, local.y, local.z,
                                       160);
    }
}
