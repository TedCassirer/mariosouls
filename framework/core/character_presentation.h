#pragma once
#include "body.h"
#include "contact.h"
#include "mesh_interpolation.h"
#include "presentation_transform.h"
#include "../sdk/render_api.h"
#include <algorithm>
#include <cmath>
#include <vector>

// Render-only CPU assembly. The host validates actor/camera identity and mesh
// buffers before borrowing them here. No native memory, device or guest calls.
namespace SCCharacterPresentation {
using Point = SCContact::Point;

inline float dot(Point a, Point b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Point cross(Point a, Point b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline Point unit(Point p) {
    float n = std::sqrt(dot(p, p));
    return n > .0001f ? p * (1 / n) : Point{0, 0, 1};
}

inline void identity(float m[16]) {
    std::fill(m, m + 16, 0.f);
    m[0] = m[5] = m[10] = m[15] = 1;
}

// Metres, radians, row-major left-handed matrices. A backend translates these
// conventions explicitly. Lens limits/defaults are selected by the host.
struct CameraInput {
    Point eye{}, at{};
    float fov = 0, aspect = 0, nearPlane = 0, farPlane = 0;
    const float *cameraWorld = nullptr; // Optional validated rigid 4x4, borrowed.
};

inline bool camera(const CameraInput &in, SCRenderAPI::Camera &out) {
    if (!SCContact::finite(in.eye) || !SCContact::finite(in.at) || !std::isfinite(in.fov) ||
        in.fov <= 0 || in.fov >= 3.14159265f || !std::isfinite(in.aspect) || in.aspect <= 0 ||
        !std::isfinite(in.nearPlane) || !std::isfinite(in.farPlane) || in.nearPlane <= 0 ||
        in.farPlane <= in.nearPlane)
        return false;
    out = {};
    identity(out.world);
    if (in.cameraWorld) {
        if (!SCPresentation::rigid(in.cameraWorld))
            return false;
        SCPresentation::view(in.cameraWorld, out.view);
    } else {
        Point z = unit(in.at - in.eye), x = unit(cross({0, 1, 0}, z)), y = cross(z, x);
        float view[16] = {x.x,
                          y.x,
                          z.x,
                          0,
                          x.y,
                          y.y,
                          z.y,
                          0,
                          x.z,
                          y.z,
                          z.z,
                          0,
                          -dot(x, in.eye),
                          -dot(y, in.eye),
                          -dot(z, in.eye),
                          1};
        std::copy(view, view + 16, out.view);
    }
    float ys = 1 / std::tan(in.fov / 2);
    out.projection[0] = ys / in.aspect;
    out.projection[5] = ys;
    out.projection[10] = in.farPlane / (in.farPlane - in.nearPlane);
    out.projection[11] = 1;
    out.projection[14] = -in.nearPlane * out.projection[10];
    return true;
}

struct MeshInput {
    const SC_Mesh &mesh; // Validated borrowed arrays, centimetres.
    const float *feet;   // Three local centimetre components, borrowed.
    Point origin;        // Host metres, includes render-only anchor correction.
    float modelScale;
    const SCVisual::MeshHistory *history; // Optional, matching validated mesh.
    float alpha;
    const float *modelWorld; // Optional validated rigid cinematic transform.
};

inline void mesh(const MeshInput &in, std::vector<SCRenderAPI::Vertex> &out) {
    const auto &m = in.mesh;
    bool blended = in.history && in.history->matches(m);
    out.resize(size_t(m.numTrianglesUsed) * 3);
    for (size_t i = 0; i < out.size(); ++i) {
        auto &p = out[i];
        float position[3];
        for (size_t axis = 0; axis < 3; ++axis) {
            float v = blended ? in.feet[axis] + in.history->position(i * 3 + axis, in.alpha)
                              : m.position[i * 3 + axis];
            position[axis] = SCBody::modelCoordinate(v, in.feet[axis], in.modelScale) * .01f;
        }
        p.x = in.origin.x + position[0];
        p.y = in.origin.y + position[1];
        p.z = in.origin.z + position[2];
        int r = std::clamp(int(m.color[i * 3] * 255), 0, 255),
            g = std::clamp(int(m.color[i * 3 + 1] * 255), 0, 255),
            b = std::clamp(int(m.color[i * 3 + 2] * 255), 0, 255);
        p.color = 0xff000000 | (r << 16) | (g << 8) | b;
        Point normal =
            unit({blended ? in.history->normal(i * 3, in.alpha) : m.normal[i * 3],
                  blended ? in.history->normal(i * 3 + 1, in.alpha) : m.normal[i * 3 + 1],
                  blended ? in.history->normal(i * 3 + 2, in.alpha) : m.normal[i * 3 + 2]});
        if (in.modelWorld) {
            float point[3] = {p.x, p.y, p.z}, worldPoint[3], n[3] = {normal.x, normal.y, normal.z},
                  wn[3];
            SCPresentation::transform(in.modelWorld, point, worldPoint, true);
            SCPresentation::transform(in.modelWorld, n, wn, false);
            p.x = worldPoint[0];
            p.y = worldPoint[1];
            p.z = worldPoint[2];
            normal = unit({wn[0], wn[1], wn[2]});
        }
        int light = int(255 * (.4f + .6f * std::max(0.f, dot(normal, unit({-.3f, .8f, -.5f})))));
        p.shade = 0xff000000 | (light << 16) | (light << 8) | light;
        p.u = m.uv[i * 2];
        p.v = m.uv[i * 2 + 1];
    }
}
} // namespace SCCharacterPresentation
