#pragma once
#include "../sdk/character_api.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace SCVisual {
// Presentation-only history. The validated SDK mesh/state and hit sockets remain
// untouched. Vertices are stored relative to the feet, so native corrections and
// moving-platform translation do not contaminate animation interpolation.
class MeshHistory {
    std::vector<float> previous, current, previousNormals, normals, uv, color;
    SC_Attachments oldHands{}, hands{};
    double stamp = 0;
    uint64_t identity = 0;
    bool paired = false;

public:
    void clear() {
        current.clear();
        previous.clear();
        paired = false;
        hands = {};
    }

    void freeze() {
        paired = false;
    }

    void capture(const SC_Mesh &mesh, const float *feet, const SC_Attachments &sockets, double now,
                 uint64_t key) {
        size_t n = size_t(mesh.numTrianglesUsed) * 9;
        if (!n || mesh.numTrianglesUsed > mesh.capacityTriangles || !std::isfinite(now)) {
            clear();
            return;
        }
        size_t uvCount = size_t(mesh.numTrianglesUsed) * 6;
        bool compatible = identity == key && current.size() == n && now > stamp &&
                          now - stamp < .1 &&
                          !std::memcmp(uv.data(), mesh.uv, uvCount * sizeof(float)) &&
                          !std::memcmp(color.data(), mesh.color, n * sizeof(float));
        previous.swap(current);
        previousNormals.swap(normals);
        oldHands = hands;
        current.resize(n);
        for (size_t i = 0; i < n; ++i)
            current[i] = mesh.position[i] - feet[i % 3];
        normals.assign(mesh.normal, mesh.normal + n);
        uv.assign(mesh.uv, mesh.uv + uvCount);
        color.assign(mesh.color, mesh.color + n);
        hands = sockets;
        if (hands.size != sizeof(SC_Attachments) || (hands.valid & ~3u))
            hands.valid = 0;
        for (unsigned hand = 0; hand < 2; ++hand)
            for (int j = 0; j < 16; ++j)
                if (!std::isfinite((hand ? hands.left : hands.right)[j]) ||
                    std::abs((hand ? hands.left : hands.right)[j]) > 1000000)
                    hands.valid &= ~(1u << hand);
        for (float *m : {hands.right, hands.left})
            for (int j = 0; j < 3; ++j)
                m[12 + j] -= feet[j];
        // A local pose jump larger than 2 m is not an animation frame to blend.
        if (compatible)
            for (size_t i = 0; i < n; i += 3) {
                float d = 0;
                for (int j = 0; j < 3; ++j) {
                    float v = current[i + j] - previous[i + j];
                    d += v * v;
                }
                if (d > 200 * 200) {
                    compatible = false;
                    break;
                }
            }
        paired = compatible;
        stamp = now;
        identity = key;
    }

    float alpha(double now) const {
        return paired && std::isfinite(now) ? float(std::clamp((now - stamp) * SC_FIXED_HZ, 0., 1.))
                                            : 1.f;
    }

    float position(size_t index, float t) const {
        return paired ? previous[index] + (current[index] - previous[index]) * t : current[index];
    }

    float normal(size_t index, float t) const {
        return paired ? previousNormals[index] + (normals[index] - previousNormals[index]) * t
                      : normals[index];
    }

    bool matches(const SC_Mesh &mesh) const {
        return !current.empty() && current.size() == size_t(mesh.numTrianglesUsed) * 9;
    }

    // Blend transformed prop vertices with the same alpha as the body. Do not
    // interpolate matrices used by native combat or return a non-rigid hit socket.
    float socket(unsigned hand, const float *fallback, const float *feet, const float *local,
                 int axis, float t, bool direction = false) const {
        auto point = [&](const float *m) {
            float p = direction ? 0.f : m[12 + axis];
            for (int j = 0; j < 3; ++j)
                p += m[j * 4 + axis] * local[j];
            return p;
        };
        unsigned bit = 1u << hand;
        if (current.empty() || !(hands.valid & bit))
            return point(fallback) - (direction ? 0.f : feet[axis]);
        float b = point(hand ? hands.left : hands.right);
        if (!paired || !(oldHands.valid & bit))
            return b;
        float a = point(hand ? oldHands.left : oldHands.right);
        return a + (b - a) * t;
    }
};

// Smooth only the displayed native anchor; collision/camera/damage keep using
// the current native position. Stop at the target; never extrapolate into walls.
class Anchor {
    float from[3]{}, target[3]{};
    double stamp = 0;
    uint64_t identity = 0;
    bool valid = false;

public:
    void clear() {
        valid = false;
    }

    float sample(int axis, double now) const {
        float t = float(std::clamp((now - stamp) * SC_FIXED_HZ, 0., 1.));
        return from[axis] + (target[axis] - from[axis]) * t;
    }

    void observe(const float *p, double now, uint64_t key, bool snap = false) {
        float distance = 0;
        for (int j = 0; j < 3; ++j)
            distance += (p[j] - target[j]) * (p[j] - target[j]);
        snap = snap || !valid || key != identity || now < stamp || now - stamp > .1 ||
               distance > 2 * 2;
        if (!snap && distance == 0)
            return;
        for (int j = 0; j < 3; ++j) {
            from[j] = snap ? p[j] : sample(j, now);
            target[j] = p[j];
        }
        valid = true;
        identity = key;
        stamp = now;
    }
};
} // namespace SCVisual
