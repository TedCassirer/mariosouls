#pragma once

using EquipmentVertex = SCGeometry::PropVertex;

struct EquipmentProp {
    bool valid = false;
    uint32_t width = 0, height = 0;
    float bounds[6] = {};
    std::vector<EquipmentVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<uint8_t> rgba;
};

static std::unordered_map<int, EquipmentProp> equipmentProps;

static EquipmentProp *equipmentProp(int model) {
    if (model < 0 || model > 9999)
        return nullptr;
    auto found = equipmentProps.find(model);
    if (found != equipmentProps.end())
        return found->second.valid ? &found->second : nullptr;
    if (equipmentProps.size() >= 128)
        return nullptr;
    auto &p = equipmentProps[model];
    char directory[MAX_PATH] = {}, file[MAX_PATH] = {};
    GetPrivateProfileStringA("Framework", "EquipmentDirectory", "", directory, sizeof(directory),
                             ini);
    if (!directory[0])
        return nullptr;
    snprintf(file, sizeof(file), "%s\\wp_a_%04d.scprop", directory, model);
    FILE *f = fopen(file, "rb");
    if (!f)
        return nullptr;
    char magic[8] = {};
    uint32_t counts[4] = {};
    bool ok = fread(magic, 1, 8, f) == 8 && !memcmp(magic, "SCPROP1", 8) &&
              fread(counts, 4, 4, f) == 4 && fread(p.bounds, 4, 6, f) == 6;
    uint32_t nv = counts[0], nt = counts[1];
    p.width = counts[2];
    p.height = counts[3];
    ok = ok && nv > 0 && nv <= 65536 && nt > 0 && nt <= 65536 && p.width > 0 && p.width <= 2048 &&
         p.height > 0 && p.height <= 2048;
    for (int i = 0; i < 6; i++)
        ok = ok && isfinite(p.bounds[i]) && fabsf(p.bounds[i]) < 5;
    for (int i = 0; i < 3; i++)
        ok = ok && p.bounds[i] <= p.bounds[i + 3];
    if (ok) {
        p.vertices.resize(nv);
        p.indices.resize(nt * 3);
        p.rgba.resize(p.width * p.height * 4);
        ok = fread(p.vertices.data(), sizeof(EquipmentVertex), nv, f) == nv &&
             fread(p.indices.data(), 4, nt * 3, f) == nt * 3 &&
             fread(p.rgba.data(), 1, p.rgba.size(), f) == p.rgba.size() && fgetc(f) == EOF;
        for (const auto &v : p.vertices) {
            for (float n : v.position)
                ok = ok && isfinite(n) && fabsf(n) < 5;
            for (float n : v.normal)
                ok = ok && isfinite(n) && fabsf(n) <= 1.001f;
            for (float n : v.uv)
                ok = ok && isfinite(n) && fabsf(n) < 100;
        }
        for (auto index : p.indices)
            ok = ok && index < nv;
    }
    fclose(f);
    p.valid = ok;
    if (!ok) {
        p.vertices.clear();
        p.indices.clear();
        p.rgba.clear();
    }
    logMessage("Equipment prop model=%d valid=%d vertices=%u triangles=%u", model, ok, nv, nt);
    return ok ? &p : nullptr;
}

static bool equipmentSocketValid(const float *m) {
    for (int i = 0; i < 16; i++)
        if (!isfinite(m[i]))
            return false;
    for (int j = 0; j < 3; j++) {
        float n = m[j * 4] * m[j * 4] + m[j * 4 + 1] * m[j * 4 + 1] + m[j * 4 + 2] * m[j * 4 + 2];
        if (n < .95f || n > 1.05f || fabsf(m[j * 4 + 3]) > .0001f)
            return false;
    }
    for (int j = 0; j < 3; j++)
        for (int k = j + 1; k < 3; k++)
            if (fabsf(m[j * 4] * m[k * 4] + m[j * 4 + 1] * m[k * 4 + 1] +
                      m[j * 4 + 2] * m[k * 4 + 2]) > .025f)
                return false;
    Vec offset = {m[12] - state.position[0], m[13] - state.position[1], m[14] - state.position[2]};
    return dot(offset, offset) < 300 * 300 && fabsf(m[15] - 1) < .0001f;
}

static Vec equipmentPoint(const float *m, Vec local, Vec renderOrigin) {
    Vec t = {m[12], m[13], m[14]}, p = {state.position[0], state.position[1], state.position[2]};
    Vec rotation = {m[0] * local.x + m[4] * local.y + m[8] * local.z,
                    m[1] * local.x + m[5] * local.y + m[9] * local.z,
                    m[2] * local.x + m[6] * local.y + m[10] * local.z};
    return renderOrigin + (p + (t - p) * characterBody.modelScale) * .01f +
           rotation * characterBody.modelScale;
}

static void equipmentDrawProp(SCRenderAPI::Renderer &renderer, EquipmentProp &p, const float *m,
                              Vec renderOrigin, unsigned hand, SCRenderAPI::Context context) {
    if (!p.valid || !equipmentSocketValid(m))
        return;
    std::vector<SCRenderAPI::Vertex> verts;
    auto sample = [&](const float *local, int axis, bool direction) {
        float cm[3] = {local[0] * (direction ? 1.f : 100.f), local[1] * (direction ? 1.f : 100.f),
                       local[2] * (direction ? 1.f : 100.f)};
        return visualMesh.socket(hand, m, state.position, cm, axis, visualAlpha, direction);
    };
    float offset[3] = {renderOrigin.x, renderOrigin.y, renderOrigin.z};
    SCGeometry::propMesh(p.vertices, p.indices, sample, state.position, offset,
                         characterBody.modelScale, verts);
    SCRenderAPI::Material style{};
    style.image = {p.rgba.data(), p.width, p.height};
    style.color = SCRenderAPI::ColorMode::Modulate;
    style.filter = SCRenderAPI::Filter::Linear;
    style.alphaCutoff = 127;
    renderer.mesh({verts.data(), uint32_t(verts.size() / 3)}, style, context);
}
