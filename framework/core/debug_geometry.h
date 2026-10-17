#pragma once
#include "contact.h"
#include "../sdk/render_api.h"
#include <cstdint>
#include <vector>

// Read-only, bounded diagnostic geometry in host metres. No game or guest APIs.
namespace SCDebug {
using Point = SCContact::Point;

using Vertex = SCRenderAPI::LineVertex;

struct Lines {
    std::vector<Vertex> vertices;
    size_t limit = 16000;
    bool truncated = false;

    void clear() {
        vertices.clear();
        truncated = false;
    }

    void line(Point a, Point b, uint32_t color) {
        if (!SCContact::finite(a) || !SCContact::finite(b))
            return;
        if (vertices.size() / 2 >= limit) {
            truncated = true;
            return;
        }
        vertices.push_back({a.x, a.y, a.z, color});
        vertices.push_back({b.x, b.y, b.z, color});
    }

    void marker(Point p, float r, uint32_t color) {
        line(p - Point{r, 0, 0}, p + Point{r, 0, 0}, color);
        line(p - Point{0, r, 0}, p + Point{0, r, 0}, color);
        line(p - Point{0, 0, r}, p + Point{0, 0, r}, color);
    }

    void ring(Point p, float r, uint32_t color) {
        if (!std::isfinite(r) || r <= 0 || r > 50)
            return;
        for (int i = 0; i < 32; i++) {
            float a = i * .19634954f, b = (i + 1) * .19634954f;
            line(p + Point{r * std::sin(a), 0, r * std::cos(a)},
                 p + Point{r * std::sin(b), 0, r * std::cos(b)}, color);
        }
    }

    void cylinder(Point p, float radius, float low, float high, uint32_t color) {
        if (!std::isfinite(low) || !std::isfinite(high) || high < low)
            return;
        ring(p + Point{0, low, 0}, radius, color);
        ring(p + Point{0, high, 0}, radius, color);
        for (int i = 0; i < 8; i++) {
            float a = i * .78539816f;
            Point offset = {radius * std::sin(a), 0, radius * std::cos(a)};
            line(p + offset + Point{0, low, 0}, p + offset + Point{0, high, 0}, color);
        }
    }

    void capsule(const SCContact::Capsule &c, uint32_t color) {
        if (!SCContact::valid(c))
            return;
        cylinder(c.feet, c.radius, c.bottom, c.top, color);
        for (int plane = 0; plane < 2; plane++)
            for (int end = 0; end < 2; end++)
                for (int i = 0; i < 16; i++) {
                    float a = i * .19634954f, b = (i + 1) * .19634954f, sign = end ? 1.f : -1.f,
                          y = end ? c.top : c.bottom;
                    auto point = [&](float t) {
                        return c.feet + Point{plane ? 0 : c.radius * std::cos(t),
                                              y + sign * c.radius * std::sin(t),
                                              plane ? c.radius * std::cos(t) : 0};
                    };
                    line(point(a), point(b), color);
                }
    }
};

// Conservative triangle AABB/sphere test: retains large floor triangles even
// when all three vertices and the centroid lie outside the displayed radius.
inline bool nearbyTriangle(Point a, Point b, Point c, Point p, float radius) {
    if (!SCContact::finite(a) || !SCContact::finite(b) || !SCContact::finite(c) ||
        !SCContact::finite(p) || !std::isfinite(radius) || radius <= 0)
        return false;
    Point q = {std::clamp(p.x, std::min({a.x, b.x, c.x}), std::max({a.x, b.x, c.x})),
               std::clamp(p.y, std::min({a.y, b.y, c.y}), std::max({a.y, b.y, c.y})),
               std::clamp(p.z, std::min({a.z, b.z, c.z}), std::max({a.z, b.z, c.z}))};
    q = q - p;
    return q.x * q.x + q.y * q.y + q.z * q.z <= radius * radius;
}
} // namespace SCDebug
