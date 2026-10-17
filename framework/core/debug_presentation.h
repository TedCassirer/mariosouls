#pragma once
#include "debug_geometry.h"
#include "render_geometry.h"
#include <cstdio>

namespace SCDebugView {
// Host metres and packed AARRGGBB. Snapshots own values, never actors/pointers.
// Collector decides identity, eligibility, reach, layers and sampling cadence.
enum class ShapeKind { Segment, Marker, Ring, Cylinder, Capsule };

struct Shape {
    ShapeKind kind;
    SCDebug::Point a{}, b{};
    float radius = 0, low = 0, high = 0;
    uint32_t color = 0;
};

struct Observations {
    std::vector<Shape> shapes;

    void clear() {
        shapes.clear();
    }

    bool empty() const {
        return shapes.empty();
    }

    void add(Shape s) {
        if (shapes.size() < 16000)
            shapes.push_back(s);
    }

    void line(SCDebug::Point a, SCDebug::Point b, uint32_t color) {
        add({ShapeKind::Segment, a, b, 0, 0, 0, color});
    }

    void marker(SCDebug::Point a, float radius, uint32_t color) {
        add({ShapeKind::Marker, a, {}, radius, 0, 0, color});
    }

    void ring(SCDebug::Point a, float radius, uint32_t color) {
        add({ShapeKind::Ring, a, {}, radius, 0, 0, color});
    }

    void cylinder(SCDebug::Point a, float radius, float low, float high, uint32_t color) {
        add({ShapeKind::Cylinder, a, {}, radius, low, high, color});
    }

    void capsule(const SCContact::Capsule &c, uint32_t color) {
        add({ShapeKind::Capsule, c.feet, {}, c.radius, c.bottom, c.top, color});
    }
};

// Output belongs to the caller; renderer borrows it for one synchronous call.
inline void geometry(const Observations &in, SCDebug::Lines &out) {
    out.clear();
    for (const auto &s : in.shapes) {
        switch (s.kind) {
        case ShapeKind::Segment:
            out.line(s.a, s.b, s.color);
            break;
        case ShapeKind::Marker:
            out.marker(s.a, s.radius, s.color);
            break;
        case ShapeKind::Ring:
            out.ring(s.a, s.radius, s.color);
            break;
        case ShapeKind::Cylinder:
            out.cylinder(s.a, s.radius, s.low, s.high, s.color);
            break;
        case ShapeKind::Capsule:
            out.capsule({s.a, s.low, s.high, s.radius}, s.color);
            break;
        }
    }
}

// The collector translates native/guest states into already-selected labels.
// Strings are borrowed only during panel(), which produces owned CPU vertices.
struct Panel {
    int layer = 0;
    bool xray = true;
    const char *attackState = "CLOSED", *attackName = "NONE";
    int hp = 0, stamina = 0, actors = 0, objects = 0, triangles = 0;
    bool terrainCapped = false;
    float radius = 12;
    int right = 0, left = 0, combo = 0, age = 0;
    bool guard = false;
};

struct PanelGeometry {
    std::vector<SCRenderAPI::ScreenVertex> background, text;
};

inline void panel(const Panel &in, PanelGeometry &out) {
    out.background.clear();
    out.text.clear();
    SCGeometry::quad(out.background, 581, 17, 431, 258, 0xff101b24);
    auto line = [&](float y, uint32_t color, const char *s) {
        float x = 590;
        for (; *s && x < 1000; ++s, x += 7) {
            unsigned c = (unsigned char)*s;
            if (c < 32 || c > 126)
                c = '?';
            SCGeometry::quad(out.text, x, y, 16, 16, color, (c % 16) / 16.f, (c / 16) / 8.f,
                             1 / 16.f, 1 / 8.f);
        }
    };
    const char *layers[] = {"COMBAT", "TERRAIN", "OBJECTS", "ALL"};
    char s[160];
    snprintf(s, sizeof(s), "FRAMEWORK DEBUG / %s / %s", layers[std::clamp(in.layer, 0, 3)],
             in.xray ? "THROUGH WALLS" : "DEPTH TEST");
    line(24, 0xffffffff, s);
    line(43, 0xffb8c5cf, "F8 ON/OFF F9 VIEW F10 XRAY");
    line(64, 0xff45dcff, "CYAN: NATIVE MOVEMENT CAPSULE");
    line(80, 0xffff63d5, "PINK: SECONDARY BODY CONTACT CAPSULE");
    line(96, 0xffff6666, "RED: ENEMY BODY   PURPLE: OTHER ACTOR");
    line(112, 0xffffbd46, "AMBER: ATTACK LIMIT   GREEN: PROBE SAMPLES");
    line(128, 0xffffffff, "WHITE: HP-CONFIRMED HIT   YELLOW: CONTACT");
    line(144, 0xff38cba2, "TEAL: CACHED TERRAIN   ORANGE: OBJECT ORIGIN");
    snprintf(s, sizeof(s), "ATTACK %s / %s", in.attackState, in.attackName);
    line(166, 0xffffffff, s);
    snprintf(s, sizeof(s), "HP %d   STAMINA %d   ACTORS %d   OBJECTS %d", in.hp, in.stamina,
             in.actors, in.objects);
    line(182, 0xffffffff, s);
    snprintf(s, sizeof(s), "TERRAIN %d%s   RADIUS %.0fM   SAMPLE GRID APPROX.", in.triangles,
             in.terrainCapped ? " (CAPPED)" : "", in.radius);
    line(198, 0xffb8c5cf, s);
    snprintf(s, sizeof(s), "EQUIP R:%d L:%d  SWING:%d/%d  NATIVE GUARD:%s", in.right, in.left,
             in.combo, in.age, in.guard ? "YES" : "NO");
    line(251, 0xffb8c5cf, s);
    line(219, 0xffffd38c, "BODY CAPSULES ARE NOT PER-BONE HURTBOXES.");
    line(235, 0xffb8c5cf, "ENEMY WEAPON BOXES / OBJECT BOUNDS NOT MAPPED.");
}
} // namespace SCDebugView
