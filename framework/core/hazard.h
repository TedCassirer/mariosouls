#pragma once
#include <cmath>

namespace SCHazard {
struct Point {
    float x, y, z;

    Point operator-(Point b) const {
        return {x - b.x, y - b.y, z - b.z};
    }

    Point operator+(Point b) const {
        return {x + b.x, y + b.y, z + b.z};
    }

    Point operator*(float s) const {
        return {x * s, y * s, z * s};
    }
};

struct Triangle {
    Point p[3];
};

inline float dot(Point a, Point b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Point cross(Point a, Point b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline bool finite(Point p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}

// Swept feet contact, independent of winding and character landing protection.
// No radius expansion: standing on safe terrain beside a hazard is not a hit.
inline bool touches(Point from, Point to, const Triangle &t) {
    if (!finite(from) || !finite(to))
        return false;
    for (auto p : t.p)
        if (!finite(p))
            return false;
    auto u = t.p[1] - t.p[0], v = t.p[2] - t.p[0], n = cross(u, v);
    float nn = dot(n, n);
    if (nn < 1e-8f || !std::isfinite(nn))
        return false;
    float a = dot(from - t.p[0], n), b = dot(to - t.p[0], n), eps = .02f * std::sqrt(nn);
    if ((a > eps && b > eps) || (a < -eps && b < -eps))
        return false;
    float delta = a - b;
    Point hit = std::fabs(delta) > 1e-7f
                    ? from + (to - from) * std::fmax(0.f, std::fmin(1.f, a / delta))
                    : to;
    hit = hit - n * (dot(hit - t.p[0], n) / nn);
    auto w = hit - t.p[0];
    float uu = dot(u, u), uv = dot(u, v), vv = dot(v, v), wu = dot(w, u), wv = dot(w, v),
          den = uu * vv - uv * uv;
    if (den <= 1e-8f)
        return false;
    float s = (vv * wu - uv * wv) / den, r = (uu * wv - uv * wu) / den;
    return s >= -1e-5f && r >= -1e-5f && s + r <= 1.00001f;
}
} // namespace SCHazard
