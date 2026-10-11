#undef NDEBUG
#include "../core/feedback.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace SCFeedback;
    Receipt r{0x10000, 42, 7, 0, 100, 1000};
    assert(r.sample(42, 100, true, 1000) == 0);
    assert(r.sample(42, 99, true, 1033) == 1);
    assert(r.sample(42, 0, true, 1067) == 1);
    assert(r.sample(43, 0, true, 1033) == -1);
    assert(r.sample(42, 0, false, 1033) == -1);
    assert(r.sample(42, 0, true, 1181) == -1);
    assert(r.sample(42, 110, true, 1033) == -1);
    r.began = 0xfffffff0;
    assert(r.sample(42, 90, true, 16) == 1);
    Group group;
    assert(group.first(7));
    assert(!group.first(7));
    assert(group.first(8));
    assert(!group.first(7));
    auto light = envelope(.65f), heavy = envelope(1.65f);
    assert(light.stopMs == 34 && heavy.stopMs == 67);
    assert(envelope(99).stopMs <= 80);
    assert(heavy.rumbleMs > light.rumbleMs);
    Pause pause;
    SC_Input input{};
    pause.start(1000, 34);
    input.attack = 1;
    assert(pause.filter(input, 1010));
    input = {};
    assert(pause.filter(input, 1030));
    assert(!pause.filter(input, 1034) && input.attack);
    input = {};
    assert(!pause.filter(input, 1035) && !input.attack);
    pause.start(2000, 500);
    input.jump = 1;
    assert(pause.filter(input, 2001));
    input = {};
    assert(!pause.filter(input, 2200) && !input.jump);
    pause.start(3000, 50);
    input.attack = 1;
    pause.filter(input, 3001);
    pause.clear();
    input = {};
    assert(!pause.filter(input, 3002) && !input.attack);
    assert(motor(40000, 30000, true) == 40000);
    assert(motor(0, 30000, true) == 30000);
    assert(motor(123, 30000, false) == 123);
    int16_t pcm[12480];
    sound(pcm, 6240, 48000, 1, 7, .7f);
    double energy = 0;
    for (int i = 0; i < 6240; i++) {
        assert(pcm[i * 2] == pcm[i * 2 + 1]);
        assert(abs(pcm[i * 2]) < 31130);
        energy += double(pcm[i * 2]) * pcm[i * 2];
    }
    assert(energy > 1e9);
    assert(pcm[0] == 0 && abs(pcm[12478]) < 10);
    puts(
        "PASS feedback: confirmed damage, lethal/late/recycled targets, wraparound, area-hit grouping, weighted bounded pause, input buffering/cancellation, rumble mixing, PCM bounds");
}
