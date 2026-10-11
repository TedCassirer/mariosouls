#include "../core/debug_geometry.h"
#include <cassert>
#include <limits>

int main() {
    using namespace SCDebug;
    assert(nearbyTriangle({-100, 0, -100}, {100, 0, -100}, {0, 0, 100}, {0, 1, 0}, 2));
    assert(!nearbyTriangle({-2, 10, -2}, {2, 10, -2}, {0, 10, 2}, {0, 1, 0}, 2));
    float nan = std::numeric_limits<float>::quiet_NaN();
    assert(!nearbyTriangle({nan, 0, 0}, {}, {}, {}, 2));
    Lines lines;
    lines.limit = 3;
    lines.marker({}, 1, 0xffffffff);
    assert(lines.vertices.size() == 6);
    lines.line({}, {1, 0, 0}, 0);
    assert(lines.vertices.size() == 6 && lines.truncated);
    lines.clear();
    lines.line({nan, 0, 0}, {}, 0);
    assert(lines.vertices.empty() && !lines.truncated);
    lines.limit = 1000;
    lines.capsule({{10, 20, 30}, .22f, .83f, .22f}, 0);
    assert(!lines.vertices.empty());
    for (auto v : lines.vertices) {
        assert(v.x >= 9.779f && v.x <= 10.221f);
        assert(v.y >= 19.999f && v.y <= 21.051f);
        assert(v.z >= 29.779f && v.z <= 30.221f);
    }
    lines.clear();
    lines.capsule({{}, 2, 1, .2f}, 0);
    assert(lines.vertices.empty());
}
