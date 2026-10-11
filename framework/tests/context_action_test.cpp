#undef NDEBUG
#include "../core/context_action.h"
#include "../hosts/ds1ptde/interaction.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main() {
    SCContext::Button button;
    auto tick = [&](bool down, bool prompt = false, bool guest = true, bool focus = true,
                    uintptr_t actor = 1, int slot = 0) {
        return button.update(down, focus, actor, slot, guest, prompt);
    };
    tick(false);
    auto r = tick(true, true);
    assert(r.interact && !r.jump && r.blockNative);
    r = tick(true, false);
    assert(!r.interact && !r.jump); // Prompt vanishes after recovery.
    r = tick(true, true, false);
    assert(!r.interact && !r.jump && r.blockNative); // Opening a menu cannot confirm it too.
    tick(false, true, false);
    r = tick(true, true, false);
    assert(!r.jump && !r.interact && !r.blockNative);
    r = tick(true, false, true);
    assert(!r.jump && !r.interact); // Held Leave confirmation cannot jump.
    tick(false);
    r = tick(true);
    assert(r.jump && !r.interact);
    r = tick(true, true);
    assert(r.jump && !r.interact); // A prompt appearing mid-jump cannot steal its hold.
    r = tick(true, true, false);
    assert(!r.jump && r.blockNative);
    r = tick(true, false);
    assert(!r.jump); // Native interruption consumes the press.
    tick(false);
    tick(true, true);
    tick(true, true, true, false);
    r = tick(true, true);
    assert(!r.interact && !r.jump); // Refocus with A held.
    tick(false);
    r = tick(true, true);
    assert(r.interact);
    r = tick(true, true, true, true, 2);
    assert(!r.interact && !r.jump); // Actor replaced.
    tick(false, false, true, true, 2);
    r = tick(true, false, true, true, 2);
    assert(r.jump);
    r = tick(true, true, true, true, 2, 1);
    assert(!r.interact && !r.jump); // Controller replaced.
    r = tick(true, true, true, true, 2, -1);
    assert(!r.interact && !r.jump);
    assert(HostInteraction::idleAnimation(100000, 0));
    assert(HostInteraction::idleAnimation(120000, 0)); // Reported live rejection.
    assert(HostInteraction::idleAnimation(20000, 0));
    for (int id : {-1, 120001, 120500, 107000, 107700, 9005})
        assert(!HostInteraction::idleAnimation(id, 0));
    assert(!HostInteraction::idleAnimation(120000, 0xffffffff));
    puts(
        "PASS contextual A: interaction/jump exclusivity, held presses, native menus, interruption, focus/actor/controller changes; stance idle regression");
}
