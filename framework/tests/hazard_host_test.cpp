#undef NDEBUG
#include <vector>
#include <cassert>
#include <cstdio>
#include "../core/hazard.h"
using Vec = SCHazard::Point;
#include "../hosts/ds1ptde/hazards.h"

int main() {
    assert(hazardTrigger(13) && !hazardLethal(13));
    assert(hazardLethal(14) && hazardLethal(15));
    assert(!hazardTrigger(8) && !hazardLethal(8));
    assert(!guestTerrainFilter(11) && !guestTerrainFilter(19));
    assert(!guestTerrainFilter(13));
    for (unsigned filter : {0u, 8u, 9u, 10u, 12u, 14u, 15u, 17u, 18u, 20u, 21u, 22u, 23u})
        assert(guestTerrainFilter(filter));
    mapHazards.push_back({{{-10, 0, -10}, {10, 0, -10}, {0, 0, 10}}});
    hazardStart({0, 5, 0});
    assert(!hazardObserve({0, 1, 0}));
    assert(hazardObserve({0, -1, 0}));
    hazardStart({0, 5, 0}); // Native ladder/warp/action handback establishes a new origin.
    hazardStart({0, -5, 0});
    assert(!hazardObserve({0, -6, 0}));
    hazardPreviousValid = false;
    assert(!hazardObserve({0, 6, 0}));
    assert(hazardObserve({0, 0, 0}));
    puts(
        "PASS host hazards: camera is nonlethal, native lethal filters, sweep, handback/warp reset");
}
