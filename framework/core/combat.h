#pragma once
#include "../sdk/character_api.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace SCCombat {
constexpr int None = -1;

inline bool pending(uint32_t until, uint32_t now) {
    return until != 0 && int32_t(until - now) > 0;
}

inline int fallDamage(float drop, int maxHP, float safe = 8, float lethal = 20) {
    if (drop <= safe)
        return 0;
    if (drop >= lethal)
        return maxHP;
    return int(std::ceil(maxHP * .5f * (drop - safe) / (lethal - safe)));
}

struct Settings {
    float base = 100, upgradeStep = 10, fixedPower = 0, dexterityWeight = .5f;
    int upgrade = 0;
    bool scaling = true;
    float multiplier[SC_MAX_ATTACKS] = {}, poise[SC_MAX_ATTACKS] = {};
    int cost[SC_MAX_ATTACKS] = {};
};

inline float strengthBonus(int s) {
    s = std::clamp(s, 1, 99);
    return std::max(0, std::min(s, 20) - 10) * 3.f + std::max(0, std::min(s, 40) - 20) * 2.f +
           std::max(0, s - 40) * .5f;
}

inline float power(const Settings &s, int str, int dex) {
    return std::clamp(
        s.fixedPower > 0
            ? s.fixedPower
            : s.base + s.upgradeStep * std::clamp(s.upgrade, 0, 15) +
                  (s.scaling ? strengthBonus(str) + strengthBonus(dex) * s.dexterityWeight : 0),
        1.f, 1200.f);
}

struct Window {
    int move = None, age = 0, serial = 0;
    bool paid = false, exhausted = false, automaticUsed = false;
    float attackPower = 0;
    int charged = 0;
    std::unordered_set<int> hit;

    void reset() {
        move = None;
        age = 0;
        paid = exhausted = automaticUsed = false;
        charged = 0;
        attackPower = 0;
        hit.clear();
    }

    bool observe(int next, bool grounded, int automatic) {
        if (grounded)
            automaticUsed = false;
        if (move != None && move == automatic && next == None && !grounded) {
            ++age;
            return false;
        }
        if (next == move) {
            ++age;
            return false;
        }
        move = next;
        age = 0;
        paid = exhausted = false;
        charged = 0;
        attackPower = 0;
        hit.clear();
        if (next != None) {
            ++serial;
            return true;
        }
        return false;
    }

    bool beginAutomatic(int id) {
        if (id < 0 || automaticUsed || move != None)
            return false;
        automaticUsed = true;
        move = id;
        age = 0;
        ++serial;
        paid = exhausted = false;
        charged = 0;
        hit.clear();
        return true;
    }

    bool pay(int stamina, int cost, float attack) {
        if (paid || exhausted)
            return false;
        charged = 0;
        exhausted = stamina < cost;
        if (exhausted)
            return false;
        paid = true;
        charged = cost;
        attackPower = attack;
        return true;
    }

    bool active(const SC_State &s, const SC_Descriptor &d) const {
        if (move < 0 || move >= int(d.attackCount) || !paid || exhausted)
            return false;
        const auto &a = d.attacks[move];
        if (a.activeTicks && age >= int(a.activeTicks))
            return false;
        return a.flags & SC_ATTACK_AUTOMATIC ? s.velocity[1] < 0
                                             : (s.attackId == move && (s.flags & SC_ATTACK_ACTIVE));
    }

    bool canHit(int id, const SC_Descriptor &d) const {
        return move >= 0 && move < int(d.attackCount) && paid && !exhausted && !hit.count(id) &&
               hit.size() < d.attacks[move].maxTargets;
    }
};
} // namespace SCCombat
