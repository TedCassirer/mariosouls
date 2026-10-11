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
static IDirect3DTexture9 *debugFont = nullptr;

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
    return debugEnabled && player && posData && cam && currentMap() >= 0 &&
           chain(gameSlots.player, {0, 4, 0}) == player &&
           chain(player + 0x28, {0, 0x1c}) == posData && read<int>(player + 0x2d4) > 0;
}

static bool renderWanted() {
    return (characterPresented() && characterMesh.view.numTrianglesUsed) || debugVisible() ||
           launcherVisible();
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
        debugEnabled = GetPrivateProfileIntA("Debug", "Enabled", 0, ini) != 0;
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
        logf("DEBUG enabled=%d layer=%d through_walls=%d", debugEnabled, debugLayer, debugXray);
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
    Vec feet = read<Vec>(posData + 0x10);
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
        if (targetShapeCapsule(read<uintptr_t>(posData + 0x3c), feet, cap))
            debugLines.capsule(cap, 0xff45dcff);
        uintptr_t part = read<uintptr_t>(posData + 0x48);
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
    static DWORD lastLog = 0;
    if (DWORD(now - lastLog) > 2000) {
        lastLog = now;
        logf("DEBUG view layer=%d actors=%d objects=%d terrain=%d capped=%d lines=%u samples=%d",
             debugLayer, debugActors, debugObjects, debugTriangles, debugTerrainCapped,
             unsigned(debugLines.vertices.size() / 2), debugSamples);
    }
}

static bool debugFontCreate(IDirect3DDevice9 *d) {
    if (debugFont)
        return true;
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = 256;
    info.bmiHeader.biHeight = -128;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc)
        return false;
    void *pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    HFONT font =
        CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, FIXED_PITCH, "Consolas");
    if (!bitmap || !font) {
        if (bitmap)
            DeleteObject(bitmap);
        if (font)
            DeleteObject(font);
        DeleteDC(dc);
        return false;
    }
    auto oldBitmap = SelectObject(dc, bitmap), oldFont = SelectObject(dc, font);
    memset(pixels, 0, 256 * 128 * 4);
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkColor(dc, RGB(0, 0, 0));
    for (int c = 32; c < 127; c++) {
        char s = char(c);
        TextOutA(dc, (c % 16) * 16, (c / 16) * 16, &s, 1);
    }
    GdiFlush();
    bool ok = SUCCEEDED(
        d->CreateTexture(256, 128, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &debugFont, nullptr));
    if (ok) {
        D3DLOCKED_RECT lr;
        ok = SUCCEEDED(debugFont->LockRect(0, &lr, nullptr, 0));
        if (ok) {
            for (int y = 0; y < 128; y++)
                for (int x = 0; x < 256; x++) {
                    DWORD v = ((DWORD *)pixels)[y * 256 + x];
                    ((DWORD *)((BYTE *)lr.pBits + y * lr.Pitch))[x] = ((v & 255) << 24) | 0xffffff;
                }
            debugFont->UnlockRect(0);
        } else {
            debugFont->Release();
            debugFont = nullptr;
        }
    }
    SelectObject(dc, oldFont);
    SelectObject(dc, oldBitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return ok;
}

static void debugDraw(IDirect3DDevice9 *d) {
    if (!debugVisible())
        return;
    debugGather();
    d->SetVertexShader(nullptr);
    d->SetPixelShader(nullptr);
    d->SetTexture(0, nullptr);
    d->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    d->SetRenderState(D3DRS_ZENABLE, debugXray ? FALSE : TRUE);
    d->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    d->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    d->SetRenderState(D3DRS_LIGHTING, FALSE);
    d->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    d->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    d->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    d->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    auto drawLines = [&](const SCDebug::Lines &lines) {
        if (!lines.vertices.empty())
            d->DrawPrimitiveUP(D3DPT_LINELIST, UINT(lines.vertices.size() / 2),
                               lines.vertices.data(), sizeof(SCDebug::Vertex));
    };
    if (debugHas(1))
        drawLines(debugTerrain);
    drawLines(debugLines);
    if (debugHas(0))
        drawLines(debugAttack);
    // Fixed-size atlas in the existing pre-HUD composite. Managed resources survive
    // D3D9 Reset; no new default-pool resources or native UI hooks are required.
    if (!debugFontCreate(d))
        return;

    struct V {
        float x, y, z, w;
        DWORD color;
        float u, v;
    };

    std::vector<V> text;
    auto quad = [&](float x, float y, float w, float h, DWORD color, float u, float v, float du,
                    float dv) {
        V a = {x, y, 0, 1, color, u, v}, b = {x + w, y, 0, 1, color, u + du, v},
          c = {x, y + h, 0, 1, color, u, v + dv}, e = {x + w, y + h, 0, 1, color, u + du, v + dv};
        text.insert(text.end(), {a, b, c, b, e, c});
    };
    auto line = [&](float y, DWORD color, const char *s) {
        float x = 590;
        for (; *s && x < 1000; s++, x += 7) {
            unsigned c = (unsigned char)*s;
            if (c < 32 || c > 126)
                c = '?';
            quad(x, y, 16, 16, color, (c % 16) / 16.f, (c / 16) / 8.f, 1 / 16.f, 1 / 8.f);
        }
    };
    d->SetRenderState(D3DRS_ZENABLE, FALSE);
    d->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    quad(581, 17, 431, 258, 0xff101b24, 0, 0, 0, 0);
    d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, text.data(), sizeof(V));
    text.clear();
    const char *layers[] = {"COMBAT", "TERRAIN", "OBJECTS", "ALL"};
    char s[160];
    snprintf(s, sizeof(s), "FRAMEWORK DEBUG / %s / %s", layers[debugLayer],
             debugXray ? "THROUGH WALLS" : "DEPTH TEST");
    line(24, 0xffffffff, s);
    line(43, 0xffb8c5cf, "F8 ON/OFF F9 VIEW F10 XRAY F11 REPORT");
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
    d->SetTexture(0, debugFont);
    d->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    d->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    d->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    d->SetRenderState(D3DRS_ALPHAREF, 100);
    d->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    d->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    d->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, UINT(text.size() / 3), text.data(), sizeof(V));
}
