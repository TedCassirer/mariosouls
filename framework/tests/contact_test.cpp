#undef NDEBUG
#include "../core/contact.h"
#include <cassert>
#include <cstdio>
using namespace SCContact;

int main() {
    Capsule boss = {{0, 0, 0}, 2.5f, 5, 2.5f};
    assert(valid(boss));
    Point player = {0, 0, 3.1f};
    auto hit = nearest(boss, player + Point{0, .7f, 0});
    assert(inReach(player, hit, 1.25f, -2, 1.4f));
    assert(!inReach(player, boss.feet, 1.25f, -2, 1.4f));
    assert(!inReach({0, 0, 5}, nearest(boss, {0, .7f, 5}), 1.25f, -2, 1.4f));
    // A boss on the roof is not hittable from its X/Z projection.
    boss.feet.y = 25;
    assert(!inReach(player, nearest(boss, {0, .7f, 3.1f}), 1.25f, -2, 1.4f));
    boss.feet.y = 0;
    auto top = upper(boss, {0, 8, 0});
    assert(top.y == 7.5f);
    assert(top.y > 2.42f);
    Capsule hollow = {{0, 0, 0}, .45f, 1.05f, .45f};
    assert(valid(hollow));
    assert(inReach({0, 0, 1}, nearest(hollow, {0, .7f, 1}), 1.25f, -2, 1.4f));
    auto overlapping = nearest(hollow, {0, .7f, .3f});
    assert(overlapping.z < .3f && overlapping.z > 0);
    hollow.radius = NAN;
    assert(!valid(hollow));
    hollow.radius = 100;
    assert(!valid(hollow));
    assert(!inReach({0, 0, 0}, {NAN, 0, 0}, 1, -2, 2));
    puts(
        "PASS contact volumes: large body surface reach, out of range, roof separation, upper-body stomps, small bodies, overlap facing, malformed bounds");
}
