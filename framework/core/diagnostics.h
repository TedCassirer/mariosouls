#pragma once
// Structured, bounded diagnostics. Never accepts raw logs, paths, names or memory.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <algorithm>

namespace SCDiagnostics {
constexpr size_t FrameLimit = 120, EventLimit = 96;

struct Frame {
    uint32_t time = 0, generation = 0, action = 0, flags = 0, submissions = 0, acknowledgements = 0,
             rejected = 0;
    int map = -1, cachedMap = -1, owner = 0, hp = -1, maxHP = -1, stamina = -1, maxStamina = -1;
    int animation = -1, attack = -1, attackSerial = 0, right = -1, left = -1, contacts = 0,
        triangles = 0, width = 0, height = 0;
    bool actor = false, requested = false, presented = false, rig = false, focus = false,
         controller = false, nativeAction = false;
    bool worldReady = false, supported = false, combat = false, combatReady = false,
         attackExhausted = false;
    bool positionValid = false, guestValid = false;
    float position[3]{}, guestPosition[3]{}, floor = 0, speed = 0;
};
enum class Kind {
    Map,
    Owner,
    Actor,
    HealthLoss,
    Heal,
    Attack,
    CacheMismatch,
    RigMissing,
    FrameRejected,
    MotionRejected,
    HitConfirmed,
    StaminaCharged,
    AttackExhausted
};

inline const char *name(Kind k) {
    switch (k) {
    case Kind::Map:
        return "map_changed";
    case Kind::Owner:
        return "owner_changed";
    case Kind::Actor:
        return "actor_availability";
    case Kind::HealthLoss:
        return "health_loss";
    case Kind::Heal:
        return "healing";
    case Kind::Attack:
        return "attack_changed";
    case Kind::CacheMismatch:
        return "collision_map_mismatch";
    case Kind::RigMissing:
        return "requested_rig_missing";
    case Kind::FrameRejected:
        return "adapter_frame_rejected";
    case Kind::MotionRejected:
        return "motion_rejected";
    case Kind::HitConfirmed:
        return "native_hp_hit_confirmed";
    case Kind::StaminaCharged:
        return "combat_stamina_charged";
    case Kind::AttackExhausted:
        return "attack_exhausted";
    }
    return "unknown";
}

struct Event {
    uint32_t time = 0;
    Kind kind = Kind::Map;
    int before = 0, after = 0;
};

inline std::string identifier(const std::string &s) {
    if (s.empty() || s.size() > 47)
        return "unknown";
    for (char c : s)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'))
            return "unknown";
    return s;
}

inline bool finitePosition(const float *p) {
    for (int i = 0; i < 3; i++)
        if (!std::isfinite(p[i]) || std::fabs(p[i]) > 1000000)
            return false;
    return true;
}

inline std::string jsonString(const std::string &s) {
    std::string out = "\"";
    const char *hex = "0123456789abcdef";
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += char(c);
        } else if (c < 32 || c == '<' || c == '>' || c == '&' || c >= 127) {
            out += "\\u00";
            out += hex[c >> 4];
            out += hex[c & 15];
        } else
            out += char(c);
    }
    return out + '"';
}

inline std::string html(const std::string &s) {
    std::string out;
    for (char c : s) {
        switch (c) {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '"':
            out += "&quot;";
            break;
        case '\'':
            out += "&#39;";
            break;
        default:
            out += c;
        }
    }
    return out;
}

inline std::string number(float n) {
    if (!std::isfinite(n) || std::fabs(n) > 1000000)
        return "null";
    char b[48];
    snprintf(b, sizeof(b), "%.3f", double(n));
    return b;
}

inline std::string point(const float *p, bool valid) {
    return valid && finitePosition(p)
               ? "[" + number(p[0]) + "," + number(p[1]) + "," + number(p[2]) + "]"
               : "null";
}

inline std::string frameJSON(const Frame &f) {
    std::string out = "{";
    auto n = [&](const char *key, auto value) {
        out += jsonString(key) + ":" + std::to_string(value) + ",";
    };
    n("time_ms", f.time);
    n("generation", f.generation);
    n("map", f.map);
    n("cached_map", f.cachedMap);
    n("owner", f.owner);
    n("hp", f.hp);
    n("max_hp", f.maxHP);
    n("stamina", f.stamina);
    n("max_stamina", f.maxStamina);
    n("action", f.action);
    n("animation", f.animation);
    n("attack", f.attack);
    n("attack_serial", f.attackSerial);
    n("state_flags", f.flags);
    n("right_equipment", f.right);
    n("left_equipment", f.left);
    n("contacts", f.contacts);
    n("terrain_triangles", f.triangles);
    n("scene_width", f.width);
    n("scene_height", f.height);
    n("motion_submissions", f.submissions);
    n("motion_acknowledgements", f.acknowledgements);
    n("motion_rejections", f.rejected);
    auto b = [&](const char *key, bool value) {
        out += jsonString(key) + (value ? ":true," : ":false,");
    };
    b("actor_available", f.actor);
    b("requested", f.requested);
    b("presented", f.presented);
    b("rig_available", f.rig);
    b("focused", f.focus);
    b("controller_connected", f.controller);
    b("native_action_owned", f.nativeAction);
    b("world_ready", f.worldReady);
    b("guest_state_available", f.guestValid);
    b("native_support", f.supported);
    b("combat_enabled", f.combat);
    b("damage_pipeline_ready", f.combatReady);
    b("attack_exhausted", f.attackExhausted);
    out += "\"native_position\":" + point(f.position, f.positionValid) +
           ",\"guest_position\":" + point(f.guestPosition, f.guestValid);
    out += ",\"guest_floor\":" + (f.guestValid ? number(f.floor) : "null") +
           ",\"guest_speed\":" + (f.guestValid ? number(f.speed) : "null") + "}";
    return out;
}

class Journal {
    std::array<Frame, FrameLimit> frames{};
    std::array<Event, EventLimit> events{};
    size_t frameCount = 0, eventCount = 0, frameNext = 0, eventNext = 0;
    Frame previous{};
    bool seen = false, mismatch = false, missing = false;
    uint32_t lastSample = 0;

public:
    void event(Kind kind, uint32_t now, int before = 0, int after = 0) {
        events[eventNext] = {now, kind, before, after};
        eventNext = (eventNext + 1) % EventLimit;
        eventCount = std::min(eventCount + 1, EventLimit);
    }

    void observe(Frame f) {
        f.positionValid = f.positionValid && f.actor && finitePosition(f.position);
        f.guestValid = f.guestValid && f.rig && f.actor && finitePosition(f.guestPosition);
        if (seen) {
            if (f.map != previous.map)
                event(Kind::Map, f.time, previous.map, f.map);
            if (f.owner != previous.owner || f.generation != previous.generation)
                event(Kind::Owner, f.time, previous.owner, f.owner);
            if (f.actor != previous.actor)
                event(Kind::Actor, f.time, previous.actor, f.actor);
            // Replaced actors / respawns are lifecycle changes, not healing or damage.
            if (f.actor && previous.actor && f.generation == previous.generation && f.hp >= 0 &&
                previous.hp >= 0 && f.hp != previous.hp)
                event(f.hp < previous.hp ? Kind::HealthLoss : Kind::Heal, f.time, previous.hp,
                      f.hp);
            if (f.attack != previous.attack || f.attackSerial != previous.attackSerial)
                event(Kind::Attack, f.time, previous.attack, f.attack);
        }
        bool badCache = f.actor && f.map >= 0 && f.rig && f.cachedMap != f.map;
        bool badRig = f.actor && f.requested && !f.rig;
        if (badCache && !mismatch)
            event(Kind::CacheMismatch, f.time, f.cachedMap, f.map);
        if (badRig && !missing)
            event(Kind::RigMissing, f.time);
        mismatch = badCache;
        missing = badRig;
        if (!seen || uint32_t(f.time - lastSample) >= 250) {
            frames[frameNext] = f;
            frameNext = (frameNext + 1) % FrameLimit;
            frameCount = std::min(frameCount + 1, FrameLimit);
            lastSample = f.time;
        }
        previous = f;
        seen = true;
    }

    std::string json(const std::string &character, const std::string &version,
                     const std::string &commit, const std::string &source, bool dirty) const {
        std::string out =
            "{\"schema\":1,\"host\":\"ds1ptde\",\"character\":" +
            jsonString(identifier(character)) + ",\"version\":" + jsonString(version) +
            ",\"commit\":" + jsonString(commit) + ",\"source_fingerprint\":" + jsonString(source) +
            ",\"dirty_build\":" + (dirty ? "true" : "false") +
            ",\"current\":" + frameJSON(previous) + ",\"frames\":[";
        for (size_t i = 0; i < frameCount; i++) {
            if (i)
                out += ',';
            out += frameJSON(frames[(frameNext + FrameLimit - frameCount + i) % FrameLimit]);
        }
        out += "],\"events\":[";
        for (size_t i = 0; i < eventCount; i++) {
            const auto &e = events[(eventNext + EventLimit - eventCount + i) % EventLimit];
            if (i)
                out += ',';
            out += "{\"time_ms\":" + std::to_string(e.time) +
                   ",\"event\":" + jsonString(name(e.kind)) +
                   ",\"before\":" + std::to_string(e.before) +
                   ",\"after\":" + std::to_string(e.after) + "}";
        }
        return out + "]}";
    }

    std::string body(const std::string &character, const std::string &version,
                     const std::string &commit, const std::string &source, bool dirty) const {
        const auto &f = previous;
        std::string recent = "\n- Recent diagnostic events: ";
        for (size_t i = eventCount > 6 ? eventCount - 6 : 0; i < eventCount; i++) {
            const auto &e = events[(eventNext + EventLimit - eventCount + i) % EventLimit];
            recent += std::string(name(e.kind)) + " (" + std::to_string(e.before) + " -> " +
                      std::to_string(e.after) + ") ";
        }
        return "## What happened?\nDescribe the problem here.\n\n## Expected behavior\n\n## How to reproduce\n1. \n2. \n3. \n\n## Automatic context\n"
               "- Build: " +
               version + " / " + commit + (dirty ? " (modified source)" : "") +
               "\n- Source fingerprint: " + source + "\n- Character: " + identifier(character) +
               "; host: ds1ptde; schema: 1\n- Area / cached area: " + std::to_string(f.map) +
               " / " + std::to_string(f.cachedMap) +
               "\n- Position: " + point(f.position, f.positionValid) +
               "\n- Owner: " + std::to_string(f.owner) +
               "; generation: " + std::to_string(f.generation) +
               "; rig: " + (f.rig ? "yes" : "no") + "\n- HP: " + std::to_string(f.hp) + " / " +
               std::to_string(f.maxHP) + "; stamina: " + std::to_string(f.stamina) + " / " +
               std::to_string(f.maxStamina) +
               "\n- Action / animation / attack: " + std::to_string(f.action) + " / " +
               std::to_string(f.animation) + " / " + std::to_string(f.attack) +
               "\n- Equipment (right / left): " + std::to_string(f.right) + " / " +
               std::to_string(f.left) + "\n- Scene target: " + std::to_string(f.width) + " x " +
               std::to_string(f.height) +
               "; controller connected: " + (f.controller ? "yes" : "no") +
               "\n- Damage pipeline ready: " + (f.combatReady ? "yes" : "no") +
               "; combat enabled: " + (f.combat ? "yes" : "no") +
               "\n- Motion submitted / acknowledged / rejected: " + std::to_string(f.submissions) +
               " / " + std::to_string(f.acknowledgements) + " / " + std::to_string(f.rejected) +
               recent +
               "\n\n## Other mods / evidence\nList other mods and attach report.json from the local report folder if useful. Review it first. No files are uploaded automatically.\n";
    }
};

// Fresh focus and a released key are required; holding through focus cannot fire.
class Trigger {
    bool previousFocus = false, previousDown = false, issued = false;
    uint32_t last = 0;

public:
    bool update(bool down, bool focus, uint32_t now) {
        bool edge = focus && previousFocus && down && !previousDown;
        previousFocus = focus;
        previousDown = down;
        if (!edge || (issued && uint32_t(now - last) < 10000))
            return false;
        issued = true;
        last = now;
        return true;
    }
};

inline std::string reviewHTML(const std::string &body) {
    return "<!doctype html><html lang=\"en\"><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width\"><meta http-equiv=\"Content-Security-Policy\" content=\"default-src 'none'; style-src 'unsafe-inline'; form-action https://github.com; base-uri 'none'\"><title>MarioSouls bug report</title><style>body{font:17px system-ui;max-width:850px;margin:3em auto;padding:0 20px;background:#15191e;color:#eee}input,textarea{box-sizing:border-box;width:100%;font:15px monospace;background:#222b34;color:#eee;padding:12px;border:1px solid #778}textarea{height:60vh}button{font:inherit;padding:12px;margin:16px 0;background:#b5e3bd;border:0}a{color:#b5e3bd}</style><h1>Report a MarioSouls bug</h1><p>Add what happened and how to reproduce it. Review the context below, then open GitHub. You choose whether to submit the issue there.</p><p>No raw logs, save data, account names, local paths, screenshots or memory addresses are included. <a href=\"report.json\" download>Save detailed report.json</a> if you want to attach the recent diagnostic history manually.</p><form method=\"get\" action=\"https://github.com/TedCassirer/mariosouls/issues/new\"><label>Title<input name=\"title\" value=\"[Bug] Describe the problem\" maxlength=\"160\" required></label><label>Description and context<textarea name=\"body\" maxlength=\"6000\" required>" +
           html(body) +
           "</textarea></label><button>Open GitHub issue draft</button></form><p>GitHub sign-in and repository access are required. If offline, keep this folder and open this page later. Return to the game when finished; opening this page does not pause Dark Souls.</p></html>";
}
} // namespace SCDiagnostics
