#pragma once
#include "../sdk/render_api.h"
#include <cmath>
#include <vector>

// CPU-only presentation helpers. Inputs are borrowed guest buffers; no native
// addresses, graphics objects, simulation or asset-loading policy lives here.
namespace SCGeometry {
struct PropVertex {
    float position[3], normal[3], uv[2];
};

static_assert(sizeof(PropVertex) == 32, "Rigid prop vertex layout");

template <class Sample>
inline void propMesh(const std::vector<PropVertex> &vertices, const std::vector<uint32_t> &indices,
                     Sample sample, const float feet[3], const float origin[3], float scale,
                     std::vector<SCRenderAPI::Vertex> &out) {
    out.clear();
    out.reserve(indices.size());
    float lightLength = std::sqrt(.3f * .3f + .8f * .8f + .5f * .5f);
    for (uint32_t index : indices) {
        const auto &v = vertices[index]; // Indices validated by the asset reader.
        float n[3] = {sample(v.normal, 0, true), sample(v.normal, 1, true),
                      sample(v.normal, 2, true)};
        float length = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (length > .0001f)
            for (float &component : n)
                component /= length;
        else {
            n[0] = n[1] = 0;
            n[2] = 1;
        }
        float lit = (-.3f * n[0] + .8f * n[1] - .5f * n[2]) / lightLength;
        int shade = int(255 * (.48f + .52f * (lit > 0 ? lit : 0)));
        out.push_back({origin[0] + feet[0] * .01f + sample(v.position, 0, false) * (scale * .01f),
                       origin[1] + feet[1] * .01f + sample(v.position, 1, false) * (scale * .01f),
                       origin[2] + feet[2] * .01f + sample(v.position, 2, false) * (scale * .01f),
                       uint32_t(0xff000000 | shade << 16 | shade << 8 | shade), 0xffffffff, v.uv[0],
                       v.uv[1]});
    }
}

inline uint32_t color(float r, float g, float b) {
    auto byte = [](float v) { return uint32_t(v < 0 ? 0 : v > 1 ? 255 : int(v * 255)); };
    return 0xff000000 | byte(r) << 16 | byte(g) << 8 | byte(b);
}

inline void quad(std::vector<SCRenderAPI::ScreenVertex> &out, float x, float y, float w, float h,
                 uint32_t color, float u = 0, float v = 0, float du = 0, float dv = 0) {
    using V = SCRenderAPI::ScreenVertex;
    V a{x, y, 0, 1, color, u, v}, b{x + w, y, 0, 1, color, u + du, v},
        c{x, y + h, 0, 1, color, u, v + dv}, e{x + w, y + h, 0, 1, color, u + du, v + dv};
    out.insert(out.end(), {a, b, c, b, e, c});
}

inline void fit(std::vector<SCRenderAPI::ScreenVertex> &out, SCRenderAPI::Viewport vp,
                float width = 1024, float height = 720) {
    for (auto &v : out) {
        v.x = vp.x + v.x * vp.width / width;
        v.y = vp.y + v.y * vp.height / height;
    }
}

inline bool launcherMesh(const float *positions, const float *colors, const float *uv,
                         uint32_t triangles, const float at[3], float yaw, float pitch,
                         std::vector<SCRenderAPI::Vertex> &out) {
    out.clear();
    out.reserve(size_t(triangles) * 3);
    float cy = std::cos(yaw), sy = std::sin(yaw), cp = std::cos(pitch), sp = std::sin(pitch);
    for (size_t i = 0; i < size_t(triangles) * 3; ++i) {
        float x = positions[i * 3], y = cp * positions[i * 3 + 1] + sp * positions[i * 3 + 2],
              z = -sp * positions[i * 3 + 1] + cp * positions[i * 3 + 2];
        float px = cy * x + sy * z, pz = -sy * x + cy * z;
        float length = px * px + y * y + pz * pz;
        if (!std::isfinite(length) || length > 10000000)
            return false;
        out.push_back({at[0] + px * .01f, at[1] + y * .01f, at[2] + pz * .01f,
                       color(colors[i * 3], colors[i * 3 + 1], colors[i * 3 + 2]), 0xffffffff,
                       uv[i * 2], uv[i * 2 + 1]});
    }
    return true;
}
} // namespace SCGeometry
