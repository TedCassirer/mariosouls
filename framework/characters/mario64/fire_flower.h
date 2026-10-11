#pragma once

// Original procedural Fire Flower, in the character adapter's local centimetres.
// It is only a visible prop: native DS owns all spell projectiles and damage.
static void fireFlower(SC_Mesh &mesh, const float *m) {
    auto point = [&](Vec p) {
        return Vec{m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12],
                   m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
                   m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]};
    };
    auto tri = [&](Vec a, Vec b, Vec c, Vec color) {
        if (mesh.numTrianglesUsed >= mesh.capacityTriangles)
            return;
        Vec q[] = {point(a), point(b), point(c)}, n = unit(cross(q[1] - q[0], q[2] - q[0]));
        for (int k = 0; k < 3; k++) {
            unsigned i = mesh.numTrianglesUsed * 3 + k;
            mesh.position[3 * i] = q[k].x;
            mesh.position[3 * i + 1] = q[k].y;
            mesh.position[3 * i + 2] = q[k].z;
            mesh.normal[3 * i] = n.x;
            mesh.normal[3 * i + 1] = n.y;
            mesh.normal[3 * i + 2] = n.z;
            mesh.color[3 * i] = color.x;
            mesh.color[3 * i + 1] = color.y;
            mesh.color[3 * i + 2] = color.z;
            mesh.uv[2 * i] = mesh.uv[2 * i + 1] = 1;
        }
        ++mesh.numTrianglesUsed;
    };
    // Stem and two broad leaves remain legible at gameplay distance.
    for (int side = 0; side < 6; side++) {
        float a = side * 6.2831853f / 6, b = (side + 1) * 6.2831853f / 6;
        Vec p = {1.7f * cosf(a), -7, 1.7f * sinf(a)}, q = {1.7f * cosf(b), -7, 1.7f * sinf(b)};
        tri(p, q, q + Vec{0, 29, 0}, {.08f, .65f, .12f});
        tri(p, q + Vec{0, 29, 0}, p + Vec{0, 29, 0}, {.08f, .65f, .12f});
    }
    for (float sign : {-1.f, 1.f}) {
        Vec a = {0, 0, 0}, b = {sign * 13, 9, 0}, c = {sign * 5, 11, 2}, d = {sign * 7, 3, -2};
        tri(a, b, c, {.2f, .9f, .15f});
        tri(a, d, b, {.08f, .55f, .06f});
        tri(a, c, b, {.2f, .9f, .15f});
        tri(a, b, d, {.08f, .55f, .06f});
    }
    // Concentric red / orange / pale-yellow bloom, visible from both sides.
    for (float sign : {-1.f, 1.f}) {
        const float rx[] = {15, 12, 8, 0}, ry[] = {12, 9.5f, 6, 0},
                    z[] = {0, sign * 2, sign * 3, sign * 3.4f};
        const Vec colors[] = {{1, .08f, .02f}, {1, .65f, .015f}, {1, .96f, .55f}};
        for (int band = 0; band < 3; band++)
            for (int i = 0; i < 12; i++) {
                float a = i * 6.2831853f / 12, b = (i + 1) * 6.2831853f / 12;
                Vec q[] = {{rx[band] * cosf(a), 23 + ry[band] * sinf(a), z[band]},
                           {rx[band] * cosf(b), 23 + ry[band] * sinf(b), z[band]},
                           {rx[band + 1] * cosf(b), 23 + ry[band + 1] * sinf(b), z[band + 1]},
                           {rx[band + 1] * cosf(a), 23 + ry[band + 1] * sinf(a), z[band + 1]}};
                if (sign > 0) {
                    tri(q[0], q[1], q[2], colors[band]);
                    tri(q[0], q[2], q[3], colors[band]);
                } else {
                    tri(q[0], q[2], q[1], colors[band]);
                    tri(q[0], q[3], q[2], colors[band]);
                }
            }
        for (float x : {-3.f, 3.f})
            for (int i = 0; i < 8; i++) {
                float a = i * 6.2831853f / 8, b = (i + 1) * 6.2831853f / 8;
                Vec c = {x, 24, sign * 3.6f},
                    p = {x + 1.25f * cosf(a), 24 + 3 * sinf(a), sign * 3.6f},
                    q = {x + 1.25f * cosf(b), 24 + 3 * sinf(b), sign * 3.6f};
                if (sign > 0)
                    tri(c, p, q, {.025f, .025f, .025f});
                else
                    tri(c, q, p, {.025f, .025f, .025f});
            }
    }
}

static void firePresentation(SC_Mesh &mesh, bool props) {
    if (!mario.fireAppearance)
        return;
    // ROM material colors distinguish cloth from skin, gloves and shoes. Apply to
    // fresh mesh output only, so repeated correction/posing never accumulates tint.
    for (unsigned i = 0; i < mesh.numTrianglesUsed * 3; i++) {
        float *c = mesh.color + i * 3;
        if (c[0] > .95f && c[1] < c[0] * .12f && c[2] < c[0] * .12f) {
            float v = c[0];
            c[0] = c[1] = c[2] = v;
        } else if (c[2] > .15f && c[0] < c[2] * .12f && c[1] < c[2] * .12f) {
            float v = c[2];
            c[0] = v;
            c[1] = .015f * v;
            c[2] = .025f * v;
        }
    }
    if (props) {
        if (mario.equipmentPose.rightKind == SC_EQUIPMENT_PYROMANCY &&
            (mario.attachments.valid & 1))
            fireFlower(mesh, mario.attachments.right);
        if (mario.equipmentPose.leftKind == SC_EQUIPMENT_PYROMANCY && (mario.attachments.valid & 2))
            fireFlower(mesh, mario.attachments.left);
    }
}
