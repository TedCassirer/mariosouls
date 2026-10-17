#pragma once

// Host implementation fragment, included after native hazards and event-floor handling.
// Loads edition-owned terrain and converts local surfaces; native Havok still owns walls/events.

static bool loadMap() {
    int map = currentMap();
    if (map < 0)
        return false;
    if (map == loadedMap && !mapVertices.empty())
        return true;
    char file[MAX_PATH];
    GetPrivateProfileStringA("MarioSouls", "Collision", "", file, sizeof(file), ini);
    char *slash = strrchr(file, '\\');
    if (!slash)
        return false;
    snprintf(slash + 1, sizeof(file) - (slash + 1 - file), "m%02d_%02d_00_00.bin", map / 100,
             map % 100);
    char prepared[MAX_PATH] = {}, directory[MAX_PATH] = {};
    GetPrivateProfileStringA("Framework", "CollisionDirectory", "", directory, sizeof(directory),
                             ini);
    FILE *f = nullptr;
    if (directory[0]) {
        snprintf(prepared, sizeof(prepared), "%s\\m%02d_%02d_00_00.bin", directory, map / 100,
                 map % 100);
        f = fopen(prepared, "rb");
        if (f)
            logMessage("Native-owned event collision cache: %s", prepared);
    }
    if (!f)
        f = fopen(file, "rb");
    if (!f) {
        logMessage("No collision cache for map %d", map);
        return false;
    }
    char magic[4];
    uint32_t count = 0;
    bool ok = fread(magic, 1, 4, f) == 4;
    bool typed = ok && !memcmp(magic, "MSC2", 4);
    ok = ok && (typed || !memcmp(magic, "MSC1", 4)) && fread(&count, 4, 1, f) == 1 && count > 0 &&
         count < 2000000;
    std::vector<Vec> data;
    std::vector<SCHazard::Triangle> hazards;
    if (ok)
        for (unsigned i = 0; i < count; i++) {
            uint32_t filter = 8;
            Vec triangle[3];
            if ((typed && fread(&filter, 4, 1, f) != 1) || filter > 255 ||
                fread(triangle, sizeof(Vec), 3, f) != 3) {
                ok = false;
                break;
            }
            for (auto p : triangle)
                if (!isfinite(dot(p, p)))
                    ok = false;
            if (!ok)
                break;
            if (hazardLethal(filter)) {
                SCHazard::Triangle t;
                for (int v = 0; v < 3; v++)
                    t.p[v] = {triangle[v].x, triangle[v].y, triangle[v].z};
                hazards.push_back(t);
            }
            // Retain terminal surfaces for guest floor queries (some engines reject
            // movement without a floor below). Contact is fatal before combat/another step.
            // Camera-only trigger surfaces must not stop the fall above the kill plane.
            if (guestTerrainFilter(filter))
                data.insert(data.end(), triangle, triangle + 3);
        }
    if (ok && fgetc(f) != EOF)
        ok = false;
    fclose(f);
    if (!ok || data.empty())
        return false;
    mapVertices.swap(data);
    mapHazards.swap(hazards);
    hazardPreviousValid = false;

    loadedMap = map;
    arenaLoad();
    arenaRefresh();

    return true;
}

static void rebuildSurfaces() {
    std::vector<SC_Surface> surfaces;
    surfaces.reserve(mapVertices.size() / 3);
    bool flip = GetPrivateProfileIntA("MarioSouls", "FlipWinding", 1, ini) != 0;
    for (size_t i = 0; i < mapVertices.size(); i += 3) {
        Vec center = (mapVertices[i] + mapVertices[i + 1] + mapVertices[i + 2]) * (1.f / 3);
        if (fabsf(center.x - characterSession.instance.origin.x) > 110 ||
            fabsf(center.z - characterSession.instance.origin.z) > 110 ||
            fabsf(center.y - characterSession.instance.origin.y) > 110)
            continue;
        SC_Surface surface = {};
        for (int v = 0; v < 3; v++) {
            Vec q = (mapVertices[i + (flip ? 2 - v : v)] - characterSession.instance.origin) * 100;
            surface.vertices[v][0] = (int)roundf(q.x);
            surface.vertices[v][1] = (int)roundf(q.y);
            surface.vertices[v][2] = (int)roundf(q.z);
        }
        if (SCTerrain::staticSurface(adapter.profile->capabilities, surface))
            surfaces.push_back(surface);
    }
    adapter.api->surfaces(surfaces.data(), surfaces.size(), 0);
}

static bool lineClear(Vec a, Vec b) {
    Vec direction = b - a;
    for (size_t i = 0; i < mapVertices.size(); i += 3) {
        Vec v0 = mapVertices[i], e1 = mapVertices[i + 1] - v0, e2 = mapVertices[i + 2] - v0;
        Vec h = cross(direction, e2);
        float determinant = dot(e1, h);
        if (fabsf(determinant) < 1e-7f)
            continue;
        float inv = 1 / determinant;
        Vec s = a - v0;
        float u = inv * dot(s, h);
        if (u < 0 || u > 1)
            continue;
        Vec q = cross(s, e1);
        float v = inv * dot(direction, q);
        if (v < 0 || u + v > 1)
            continue;
        float t = inv * dot(e2, q);
        if (t > .02f && t < .98f)
            return false;
    }
    return true;
}
