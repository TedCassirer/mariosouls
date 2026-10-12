#undef NDEBUG
#include "../core/timing.h"
#include "../core/motion.h"
#include <cassert>
#include <cstdio>
#include <limits>

int main() {
    // Rendering cadence is deliberately independent: only elapsed simulation
    // time advances the clock. All rates produce 30 ticks per second when drained.
    for (int hz : {15, 24, 30, 60, 144, 240}) {
        SCTiming::Clock c;
        int ticks = 0;
        for (int frame = 0; frame < hz * 10; ++frame) {
            c.advance(1. / hz);
            while (c.take(true))
                ++ticks;
            assert(!c.take(true)); // repeated draws cannot manufacture time
        }
        assert(ticks == 300);
    }
    SCTiming::Clock c;
    c.advance(20);
    assert(c.take(true));
    assert(c.take(true));
    assert(!c.take(true));
    c.advance(1);
    c.clear();
    assert(!c.take(true)); // pause discards backlog
    c.advance(-1);
    c.advance(std::numeric_limits<double>::quiet_NaN());
    c.advance(std::numeric_limits<double>::infinity());
    assert(!c.take(true));
    c.advance(1);
    assert(!c.take(false));
    assert(c.take(true));
    assert(!c.take(true));
    // Native acknowledgement gates the next tick, even if render frames continue.
    SCMotion::Mailbox box;
    box.bind();
    c.advance(1. / 30);
    assert(c.take(box.ready()));
    assert(box.publish({}, {.1f, 0, 0}, 0, 0));
    for (int i = 0; i < 240; ++i) {
        c.advance(1. / 240);
        assert(!c.take(box.ready()));
    }
    SCMotion::Ticket t;
    assert(box.consume(100, t));
    assert(!c.take(box.ready()));
    assert(box.acknowledge(t));
    assert(c.take(box.ready()));
    assert(!c.take(true));
    // Preserve PTDE's existing one-tick-per-update caller semantics at low FPS.
    int ticks = 0;
    c.clear();
    for (int i = 0; i < 15; ++i) {
        c.advance(1. / 15);
        if (c.take(true))
            ++ticks;
    }
    assert(ticks == 15);
    puts(
        "PASS variable-rate timing, bounded catch-up, pause, invalid delta, backpressure and PTDE cadence");
}
