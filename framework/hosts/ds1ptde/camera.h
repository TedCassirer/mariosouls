#pragma once
#include "../../core/camera.h"

// PTDE LockCamParam (repo slot31): camDistTarget at row+0. Only live floats
// change. Native interpolation, collision, FOV, lock-on and render matrices run.
struct CameraRow {
    int id;
    uint32_t offset;
    SCCamera::Distance distance;
};

static std::vector<CameraRow> cameraRows;
static uintptr_t cameraTable = 0;
static float cameraDefaultScale = 1.f;
static bool cameraSupported = false;

static uintptr_t cameraParams(int &count) {
    count = 0;
    uintptr_t repo = read<uint32_t>(0x13785b4);
    if (!repo || read<int>(repo + 31 * 36 + 8) <= 0)
        return 0;
    uintptr_t table = chain(repo + 31 * 36 + 12, {0, 0x20});
    char type[32] = {};
    if (!table || !get(table + 12, type) || memcmp(type, "LOCK_CAM_PARAM_ST\0", 18))
        return 0;
    count = read<uint16_t>(table + 10);
    return count > 0 && count <= 512 ? table : 0;
}

static bool cameraRowMatches(uintptr_t table, size_t index, const CameraRow &row) {
    int id = 0;
    uint32_t offset = 0;
    return get(table + 48 + index * 12, id) && get(table + 52 + index * 12, offset) &&
           id == row.id && offset == row.offset;
}

static void cameraRelease() {
    int count = 0;
    uintptr_t table = cameraParams(count);
    if (table && table == cameraTable && count == int(cameraRows.size()))
        for (size_t i = 0; i < cameraRows.size(); i++) {
            auto &r = cameraRows[i];
            float current = 0, value = 0;
            if (cameraRowMatches(table, i, r) && get(table + r.offset, current) &&
                r.distance.restore(current, value))
                put(table + r.offset, value);
        }
    cameraRows.clear();
    cameraTable = 0;
}

static void cameraConfigure() {
    cameraSupported = false;
    cameraDefaultScale = 1.f;
    auto entry =
        (SC_GetCameraProfile)GetProcAddress(adapterModule, "SoulsCharacterGetCameraProfile");
    if (!entry)
        return;
    auto p = entry(SC_CAMERA_PROFILE_VERSION, sizeof(SC_CameraProfile));
    if (!SCCamera::valid(p)) {
        logMessage("CAMERA optional profile rejected; native camera retained");
        return;
    }
    cameraSupported = true;
    cameraDefaultScale = p->distanceScale;
}

static void cameraTick() {
    if (!cameraSupported || !characterOwned() || player != ownedActor || !playerCamera) {
        cameraRelease();
        return;
    }
    char section[80], value[32], fallback[32];
    snprintf(section, sizeof(section), "Camera.%s", profile->id);
    snprintf(fallback, sizeof(fallback), "%.3f", cameraDefaultScale);
    GetPrivateProfileStringA(section, "DistanceScale", fallback, value, sizeof(value), ini);
    char *end = nullptr;
    float scale = strtof(value, &end);
    if (!end || *end || !SCCamera::validScale(scale))
        scale = 1.f;
    int count = 0;
    uintptr_t table = cameraParams(count);
    if (!table || table != cameraTable || count != int(cameraRows.size())) {
        cameraRelease();
        if (!table)
            return;
        std::vector<CameraRow> rows;
        rows.reserve(count);
        for (int i = 0; i < count; i++) {
            int id = read<int>(table + 48 + i * 12);
            uint32_t offset = read<uint32_t>(table + 52 + i * 12);
            float distance = 0;
            if (offset < uint32_t(48 + count * 12) || offset > 16 * 1024 * 1024 ||
                !get(table + offset, distance) || !isfinite(distance) || distance < .5f ||
                distance > 20.f)
                return;
            // Stock has duplicate ID200. Directory position + ID + offset is identity.
            rows.push_back({id, offset, SCCamera::Distance(distance)});
        }
        cameraTable = table;
        cameraRows.swap(rows);
    }
    for (size_t i = 0; i < cameraRows.size(); i++) {
        auto &r = cameraRows[i];
        float current = 0, next = 0;
        if (!cameraRowMatches(table, i, r) || !get(table + r.offset, current))
            continue;
        if (r.distance.next(current, scale, next) && next != current && put(table + r.offset, next))
            r.distance.written(next);
    }
}
