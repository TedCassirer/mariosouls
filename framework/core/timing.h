#pragma once
#include "../sdk/character_api.h"
#include <algorithm>
#include <cmath>

namespace SCTiming {
// Fixed-rate scheduler with at most two ticks of backlog. The caller controls
// how many ticks to consume; a native mailbox can apply backpressure. Rendering
// never advances this clock. clear() discards pause and ownership-change backlog.
class Clock {
    double time = 0;

public:
    void clear() {
        time = 0;
    }

    void advance(double dt) {
        if (std::isfinite(dt) && dt > 0)
            time = std::min(time + dt, 2. / SC_FIXED_HZ);
    }

    bool take(bool ready) {
        if (!ready) {
            time = std::min(time, 1. / SC_FIXED_HZ);
            return false;
        }
        if (time + 1e-8 < 1. / SC_FIXED_HZ)
            return false;
        time = std::min(time - 1. / SC_FIXED_HZ, 1. / SC_FIXED_HZ);
        return true;
    }
};

} // namespace SCTiming
