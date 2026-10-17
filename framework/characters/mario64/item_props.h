#pragma once

// Adapter implementation fragment. Requires the live runtime/instance; all units are guest cm.
// Procedural held props and costume tinting only; no movement, spell or damage logic.

struct Vec {
    float x, y, z;

    Vec operator+(Vec b) const {
        return {x + b.x, y + b.y, z + b.z};
    }

    Vec operator-(Vec b) const {
        return {x - b.x, y - b.y, z - b.z};
    }

    Vec operator*(float s) const {
        return {x * s, y * s, z * s};
    }
};

static Vec cross(Vec a, Vec b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static Vec unit(Vec v) {
    float n = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return n > .0001f ? v * (1 / n) : Vec{0, 0, 1};
}

// Original procedural accessory, fitted to this character's hand. The shared host
// only knows that this adapter supplies its own item prop.
static void drawFlaskProp(SC_Mesh &mesh) {
    float m[16];
    library.handMatrix(m);
    Vec axis = unit({.09665986f, -.21849427f, -.9710392f});
    Vec u = unit(cross(axis, {1, 0, 0})), v = cross(axis, u), grip = {40, 0, 0};
    auto transform = [&](Vec p) {
        return Vec{p.x * m[0] + p.y * m[4] + p.z * m[8] + m[12],
                   p.x * m[1] + p.y * m[5] + p.z * m[9] + m[13],
                   p.x * m[2] + p.y * m[6] + p.z * m[10] + m[14]};
    };
    const float heights[] = {-125, -110, -50, -15, 0, 70, 75}, radii[] = {0, 45, 42, 30, 15, 15, 0};
    for (int band = 0; band < 6; band++)
        for (int side = 0; side < 10; side++) {
            float a = side * 6.2831853f / 10, b = (side + 1) * 6.2831853f / 10;
            Vec ra = u * cosf(a) + v * sinf(a), rb = u * cosf(b) + v * sinf(b);
            Vec q[4] = {transform(grip + axis * heights[band] + ra * radii[band]),
                        transform(grip + axis * heights[band] + rb * radii[band]),
                        transform(grip + axis * heights[band + 1] + rb * radii[band + 1]),
                        transform(grip + axis * heights[band + 1] + ra * radii[band + 1])};
            for (int half = 0; half < 2; half++) {
                if (mesh.numTrianglesUsed >= mesh.capacityTriangles)
                    return;
                int ids[3] = {0, half ? 2 : 1, half ? 3 : 2};
                Vec n = unit(cross(q[ids[1]] - q[0], q[ids[2]] - q[0]));
                for (int k = 0; k < 3; k++) {
                    size_t i = mesh.numTrianglesUsed * 3 + k;
                    Vec p = q[ids[k]];
                    mesh.position[i * 3] = p.x;
                    mesh.position[i * 3 + 1] = p.y;
                    mesh.position[i * 3 + 2] = p.z;
                    mesh.normal[i * 3] = n.x;
                    mesh.normal[i * 3 + 1] = n.y;
                    mesh.normal[i * 3 + 2] = n.z;
                    mesh.color[i * 3] = 1;
                    mesh.color[i * 3 + 1] = band >= 4 ? .75f : .4f;
                    mesh.color[i * 3 + 2] = .04f;
                    mesh.uv[i * 2] = mesh.uv[i * 2 + 1] = 1;
                }
                ++mesh.numTrianglesUsed;
            }
        }
}

#include "fire_flower.h"
