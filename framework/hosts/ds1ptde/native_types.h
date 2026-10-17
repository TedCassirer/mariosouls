#pragma once
#include <cmath>

// PTDE native vectors, in metres. Not part of the adapter ABI.
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

static float dot(Vec a, Vec b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec cross(Vec a, Vec b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static Vec unit(Vec v) {
    float n = sqrtf(dot(v, v));
    return n > .0001f ? v * (1 / n) : Vec{0, 0, 1};
}
