#pragma once
#include "../../core/debug_geometry.h"

// Diagnostics only: no game-memory writes, guest ticks, contact commits or cheats.
static bool debugEnabled = false, debugXray = true;
static int debugLayer = 0, debugTriangleLimit = 3000;
static float debugRadius = 12;
static SCDebug::Lines debugLines, debugTerrain, debugAttack;
static DWORD debugTerrainTime = 0, debugAttackTime = 0;
static uintptr_t debugActor = 0;
static int debugMap = -1, debugTerrainMap = -1, debugTriangles = 0, debugActors = 0,
           debugObjects = 0, debugSamples = 0;
static size_t debugTerrainSize = 0;
static bool debugTerrainCapped = false;
static char debugAttackName[40] = "NONE";

struct DebugContact {
    Vec point;
    DWORD time;
    bool confirmed;
};

static std::vector<DebugContact> debugContacts;

static SCDebug::Point debugPoint(Vec p) {
    return {p.x, p.y, p.z};
}

static bool debugHas(int layer) {
    return debugLayer == 3 || debugLayer == layer;
}

static bool debugVisible() {
    return debugEnabled && player && playerPhysics && playerCamera && currentMap() >= 0 &&
           chain(gameSlots.player, {0, 4, 0}) == player &&
           chain(player + 0x28, {0, 0x1c}) == playerPhysics && read<int>(player + 0x2d4) > 0;
}

static bool renderWanted() {
    return cutsceneVisible() || (characterPresented() && characterMesh.view.numTrianglesUsed) ||
           debugVisible() || launcherVisible();
}

static void debugContact(Vec at, bool confirmed) {
    if (!debugEnabled || !SCContact::finite(debugPoint(at)))
        return;
    if (debugContacts.size() >= 32)
        debugContacts.erase(debugContacts.begin());
    debugContacts.push_back({at, GetTickCount(), confirmed});
}

static void debugInput(bool focus) {
    static bool initialized = false, previousFocus = false, keys[3] = {};
    if (!initialized) {
        initialized = true;
        // Always start hidden, including with older Debug.Enabled settings.
        debugEnabled = false;
        debugXray = GetPrivateProfileIntA("Debug", "ThroughWalls", 1, ini) != 0;
        debugLayer = std::clamp<int>(GetPrivateProfileIntA("Debug", "Layer", 0, ini), 0, 3);
        debugRadius =
            std::clamp<int>(GetPrivateProfileIntA("Debug", "RadiusMeters", 12, ini), 4, 40);
        debugTriangleLimit = std::clamp<int>(
            GetPrivateProfileIntA("Debug", "MaxTerrainTriangles", 3000, ini), 100, 5000);
    }
    for (int i = 0; i < 3; i++) {
        bool down = (GetAsyncKeyState(VK_F8 + i) & 0x8000) != 0,
             edge = focus && previousFocus && down && !keys[i];
        keys[i] = down;
        if (!edge)
            continue;
        if (i == 0) {
            debugEnabled = !debugEnabled;
            debugContacts.clear();
            debugAttack.clear();
            debugTerrainTime = 0;
        }
        if (i == 1) {
            debugLayer = (debugLayer + 1) % 4;
            debugTerrainTime = 0;
        }
        if (i == 2)
            debugXray = !debugXray;
        logMessage("DEBUG enabled=%d layer=%d through_walls=%d", debugEnabled, debugLayer,
                   debugXray);
    }
    previousFocus = focus;
}

static void debugTerrainGather(Vec feet, DWORD now) {
    if (!debugHas(1))
        return;
    if (loadedMap != currentMap() && !characterOwned() && !nativeActionOwned() &&
        DWORD(now - debugTerrainTime) > 1000) {
        debugTerrainTime = now;
        loadMap();
    }
    if (loadedMap != currentMap()) {
        debugTerrain.clear();
        debugTriangles = 0;
        return;
    }
    if (debugTerrainMap == loadedMap && debugTerrainSize == mapVertices.size() &&
        DWORD(now - debugTerrainTime) < 250)
        return;
    debugTerrainTime = now;
    debugTerrainMap = loadedMap;
    debugTerrainSize = mapVertices.size();
    debugTerrain.clear();
    debugTriangles = 0;
    debugTerrainCapped = false;
    for (size_t i = 0; i + 2 < mapVertices.size(); i += 3) {
        auto a = debugPoint(mapVertices[i]), b = debugPoint(mapVertices[i + 1]),
             c = debugPoint(mapVertices[i + 2]);
        if (!SCDebug::nearbyTriangle(a, b, c, debugPoint(feet), debugRadius))
            continue;
        if (debugTriangles >= debugTriangleLimit) {
            debugTerrainCapped = true;
            break;
        }
        ++debugTriangles;
        debugTerrain.line(a, b, 0xff38cba2);
        debugTerrain.line(b, c, 0xff38cba2);
        debugTerrain.line(c, a, 0xff38cba2);
    }
}

static void debugAttackGather(Vec feet, DWORD now) {
    if (!characterOwned() || nativeActionOwned()) {
        debugAttack.clear();
        return;
    }
    int move = characterWindow.move;
    const auto *a = attackDefinition(move);
    bool active = a && characterWindow.active(state, *profile);
    if (!active) {
        if (DWORD(now - debugAttackTime) > 650)
            debugAttack.clear();
        return;
    }
    debugAttack.clear();
    debugAttackTime = now;
    debugSamples = 0;
    snprintf(debugAttackName, sizeof(debugAttackName), "%s", attackName(move));
    Vec base = origin + Vec{state.position[0], state.position[1], state.position[2]} * .01f;
    float radius = a->rangeCm * .01f, low = a->minHeightCm * .01f, high = a->maxHeightCm * .01f;
    bool automatic = move == profile->automaticAttackId;
    if (automatic) {
        radius = std::min(radius, a->automaticRadiusCm * .01f);
        low = std::max(low, a->automaticMinHeightCm * .01f);
        high = std::min(high, a->automaticMaxHeightCm * .01f);
    }
    debugAttack.cylinder(debugPoint(base), radius, low, high, 0xffffbd46);
    // A bounded lattice samples the adapter's read-only predicate. These green
    // marks are eligible POINTS, not a guessed continuous mesh or a guaranteed hit.
    for (int iy = 0; iy <= 8; iy++)
        for (int ix = -6; ix <= 6; ix++)
            for (int iz = -6; iz <= 6; iz++) {
                Vec delta = {radius * ix / 6.f, low + (high - low) * iy / 8.f, radius * iz / 6.f};
                if (!SCContact::inReach({}, debugPoint(delta), radius, low, high))
                    continue;
                if (automatic && !automaticContact(delta, *a))
                    continue;
                Vec at = base + delta, local = (at - origin) * 100;
                uint32_t contact = guest->probeContact(character, local.x, local.y, local.z);
                if (!(contact & (automatic ? SC_CONTACT_AUTOMATIC : SC_CONTACT_ATTACK)))
                    continue;
                ++debugSamples;
                debugAttack.marker(debugPoint(at), .018f, 0xff90ff57);
            }
}

static void debugGather() {
    DWORD now = GetTickCount();
    Vec feet = read<Vec>(playerPhysics + 0x10);
    debugLines.clear();
    debugActors = debugObjects = 0;
    if (!SCContact::finite(debugPoint(feet)))
        return;
    if (debugActor != player || debugMap != currentMap()) {
        debugActor = player;
        debugMap = currentMap();
        debugContacts.clear();
        debugAttack.clear();
        debugTerrain.clear();
        debugTerrainMap = -1;
        debugTerrainTime = 0;
    }
    if (debugHas(0)) {
        SCContact::Capsule cap;
        if (targetShapeCapsule(read<uintptr_t>(playerPhysics + 0x3c), feet, cap))
            debugLines.capsule(cap, 0xff45dcff);
        uintptr_t part = read<uintptr_t>(playerPhysics + 0x48);
        if (read<uintptr_t>(part + 8) == player &&
            targetShapeCapsule(read<uintptr_t>(part + 0x1c), feet, cap))
            debugLines.capsule(cap, 0xffff63d5);
        uintptr_t root = read<uintptr_t>(gameSlots.player), begin = read<uintptr_t>(root + 4),
                  end = read<uintptr_t>(root + 8);
        if (begin && end >= begin && end - begin <= 16384)
            for (uintptr_t p = begin; p < end && debugActors < 48; p += 4) {
                uintptr_t actor = read<uintptr_t>(p);
                int hp = 0;
                if (!actor || actor == player || !get(actor + 0x2d4, hp) || hp <= 0 || hp > 100000)
                    continue;
                uintptr_t physics = chain(actor + 0x28, {0, 0x1c});
                Vec at;
                if (!physics || !get(physics + 0x10, at) || !SCContact::finite(debugPoint(at)))
                    continue;
                if (dot(at - feet, at - feet) > debugRadius * debugRadius ||
                    !targetCapsule(actor, at, cap))
                    continue;
                int team = read<int>(actor + 0x74);
                bool hostile = team == 6 || team == 7 || team == 9;
                debugLines.capsule(cap, hostile ? 0xffff6666 : 0xffb794ff);
                ++debugActors;
            }
        debugAttackGather(feet, now);
        if (equipmentSwing.running() && equipmentPreviousBlade) {
            debugLines.line(debugPoint(equipmentBladeA), debugPoint(equipmentBladeB),
                            equipmentSwing.active() ? 0xff90ff57 : 0xffffbd46);
            debugLines.marker(debugPoint(equipmentBladeB), .09f, 0xff90ff57);
        }
        debugContacts.erase(
            std::remove_if(debugContacts.begin(), debugContacts.end(),
                           [now](const DebugContact &c) { return DWORD(now - c.time) > 1500; }),
            debugContacts.end());
        for (const auto &c : debugContacts)
            debugLines.marker(debugPoint(c.point), c.confirmed ? .16f : .1f,
                              c.confirmed ? 0xffffffff : 0xffffbd46);
    }
    if (debugHas(2)) {
        uintptr_t manager = read<uintptr_t>(0x137d5f8), maps = read<uintptr_t>(manager + 0x20);
        int count = read<int>(manager + 0x1c), scanned = 0;
        if (maps && count > 0 && count <= 64)
            for (int m = 0; m < count && scanned < 20000 && debugObjects < 128; m++) {
                uintptr_t map = maps + m * 0xec, objects = read<uintptr_t>(map + 0x40);
                int n = read<int>(map + 0x3c);
                if (!objects || n < 1 || n > 10000)
                    continue;
                for (int j = 0; j < n && scanned < 20000 && debugObjects < 128; j++, scanned++) {
                    uintptr_t obj = objects + j * 0xc0;
                    if (!read<uintptr_t>(obj + 0x90))
                        continue;
                    float matrix[12];
                    if (!get(obj + 0x60, matrix))
                        continue;
                    Vec at = {matrix[3], matrix[7], matrix[11]};
                    if (!SCContact::finite(debugPoint(at)) ||
                        dot(at - feet, at - feet) > debugRadius * debugRadius)
                        continue;
                    // Object origins are known. Do NOT invent a box from their visual mesh.
                    debugLines.marker(debugPoint(at), .28f, 0xffffad55);
                    debugLines.ring(debugPoint(at), .2f, 0xffffad55);
                    ++debugObjects;
                }
            }
    }
    if (characterOwned() && !nativeActionOwned())
        for (const auto &c : worldContacts) {
            auto p = debugPoint(c.point);
            debugLines.marker(p, .055f, 0xffffff66);
            debugLines.line(p, debugPoint(c.point + c.normal * .45f), 0xffffff66);
        }
    debugTerrainGather(feet, now);
}

static void debugDraw(SCRenderAPI::Renderer &renderer, SCRenderAPI::Context context) {
    if (!debugVisible())
        return;
    if (debugEnabled)
        debugGather();
    auto drawLines = [&](const SCDebug::Lines &lines) {
        renderer.lines({lines.vertices.data(), uint32_t(lines.vertices.size() / 2)}, !debugXray,
                       context);
    };
    if (debugEnabled && debugHas(1))
        drawLines(debugTerrain);
    if (debugEnabled)
        drawLines(debugLines);
    if (debugEnabled && debugHas(0))
        drawLines(debugAttack);
    // Fixed-size CPU atlas; upload/resource lifetime belongs to the backend.
    if (!presentationFontCreate())
        return;

    std::vector<SCRenderAPI::ScreenVertex> text;
    auto quad = [&](float x, float y, float w, float h, uint32_t color, float u, float v, float du,
                    float dv) { SCGeometry::quad(text, x, y, w, h, color, u, v, du, dv); };
    auto line = [&](float y, DWORD color, const char *s) {
        float x = 590;
        for (; *s && x < 1000; s++, x += 7) {
            unsigned c = (unsigned char)*s;
            if (c < 32 || c > 126)
                c = '?';
            quad(x, y, 16, 16, color, (c % 16) / 16.f, (c / 16) / 8.f, 1 / 16.f, 1 / 8.f);
        }
    };
    quad(581, 17, 431, 258, 0xff101b24, 0, 0, 0, 0);
    renderer.screen({text.data(), 2}, {}, context);
    text.clear();
    const char *layers[] = {"COMBAT", "TERRAIN", "OBJECTS", "ALL"};
    char s[160];
    snprintf(s, sizeof(s), "FRAMEWORK DEBUG / %s / %s", layers[debugLayer],
             debugXray ? "THROUGH WALLS" : "DEPTH TEST");
    line(24, 0xffffffff, s);
    line(43, 0xffb8c5cf, "F8 ON/OFF F9 VIEW F10 XRAY");
    line(64, 0xff45dcff, "CYAN: NATIVE MOVEMENT CAPSULE");
    line(80, 0xffff63d5, "PINK: SECONDARY BODY CONTACT CAPSULE");
    line(96, 0xffff6666, "RED: ENEMY BODY   PURPLE: OTHER ACTOR");
    line(112, 0xffffbd46, "AMBER: ATTACK LIMIT   GREEN: PROBE SAMPLES");
    line(128, 0xffffffff, "WHITE: HP-CONFIRMED HIT   YELLOW: CONTACT");
    line(144, 0xff38cba2, "TEAL: CACHED TERRAIN   ORANGE: OBJECT ORIGIN");
    const char *attackState = !characterOwned() || nativeActionOwned()  ? "NATIVE CONTROL"
                              : characterWindow.exhausted               ? "EXHAUSTED"
                              : characterWindow.active(state, *profile) ? "ACTIVE"
                              : debugAttack.vertices.empty()            ? "CLOSED"
                                                                        : "RECENT (650MS)";
    snprintf(s, sizeof(s), "ATTACK %s / %s", attackState, debugAttackName);
    line(166, 0xffffffff, s);
    snprintf(s, sizeof(s), "HP %d   STAMINA %d   ACTORS %d   OBJECTS %d", read<int>(player + 0x2d4),
             read<int>(player + 0x2e4), debugActors, debugObjects);
    line(182, 0xffffffff, s);
    snprintf(s, sizeof(s), "TERRAIN %d%s   RADIUS %.0fM   SAMPLE GRID APPROX.",
             debugHas(1) ? debugTriangles : 0, debugTerrainCapped && debugHas(1) ? " (CAPPED)" : "",
             debugRadius);
    line(198, 0xffb8c5cf, s);
    snprintf(s, sizeof(s), "EQUIP R:%d L:%d  SWING:%d/%d  NATIVE GUARD:%s",
             equipmentCurrent.right.id, equipmentCurrent.left.id, equipmentSwing.combo + 1,
             equipmentSwing.age, equipmentConfirmedGuard ? "YES" : "NO");
    line(251, 0xffb8c5cf, s);
    line(219, 0xffffd38c, "BODY CAPSULES ARE NOT PER-BONE HURTBOXES.");
    line(235, 0xffb8c5cf, "ENEMY WEAPON BOXES / OBJECT BOUNDS NOT MAPPED.");
    SCRenderAPI::Material style{};
    style.image = presentationFont();
    style.color = SCRenderAPI::ColorMode::Modulate;
    style.alphaCutoff = 100;
    renderer.screen({text.data(), uint32_t(text.size() / 3)}, style, context);
}
