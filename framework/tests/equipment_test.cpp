#undef NDEBUG
#include "../core/equipment.h"
#include <cassert>
#include "../hosts/ds1ptde/equipment_family.h"
#include "../core/qa_identity.h"
#include <cstdio>
#include "../core/native_action.h"

int main() {
    SCNativeAction::Buttons buttons;
    assert(!buttons.sample(true, true, true, 1, 0).attack); // held at attachment
    buttons.sample(false, false, true, 1, 0);
    auto e = buttons.sample(true, true, true, 1, 0);
    assert(e.attack && e.parry);
    buttons.consume();
    assert(buttons.suppressAttack());
    assert(!buttons.sample(true, true, true, 1, 0).attack && buttons.suppressAttack());
    buttons.sample(false, false, true, 1, 0);
    assert(!buttons.suppressAttack());
    assert(!buttons.sample(true, true, false, 1, 0).attack);
    assert(!buttons.sample(true, true, true, 1, 0).attack);
    buttons.sample(false, false, true, 1, 0);
    assert(!buttons.sample(true, true, true, 2, 0).attack);
    buttons.sample(false, false, true, 2, 0);
    assert(!buttons.sample(true, true, true, 2, 1).attack);
    SCNativeAction::Completion done;
    done.begin(100);
    assert(!done.update(false, true, true, 100));
    assert(!done.update(false, true, true, 699));
    assert(done.update(false, true, true, 700));
    done.begin(100);
    assert(!done.update(true, true, true, 200));
    assert(!done.update(true, true, true, 15000));
    assert(!done.update(false, true, true, 15001));
    assert(!done.update(false, true, false, 16000));
    assert(!done.update(false, true, true, 16001));
    assert(done.update(false, true, true, 16101));
    puts(
        "PASS native action boundaries: no held/focus/device/actor replay, no timed interruption, rejected request recovery");
    assert(SCQA::meleeFixture(13, 11, 11, 11, 16, 13, 11, 11, 11));
    assert(!SCQA::meleeFixture(65, 11, 11, 11, 16, 13, 11, 11, 11));
    assert(!SCQA::meleeFixture(13, 11, 11, 11, 40, 13, 11, 11, 11));
    assert(equipmentFamily(4, 30) == SC_EQUIPMENT_AXE &&
           equipmentFamily(5, 33) == SC_EQUIPMENT_HAMMER);
    assert(equipmentFamily(1, 25) == SC_EQUIPMENT_GREATSWORD &&
           equipmentFamily(0, 20) == SC_EQUIPMENT_DAGGER);
    assert(equipmentFamily(3, 28) == SC_EQUIPMENT_CURVED_SWORD &&
           equipmentFamily(1, 23) == SC_EQUIPMENT_SWORD);
    assert(!equipmentFamily(1, 26) && !equipmentFamily(4, 32) && !equipmentFamily(5, 35) &&
           !equipmentFamily(2, 27) && !equipmentFamily(6, 36));
    for (unsigned kind : {1u, 3u, 4u, 5u, 6u, 7u}) {
        SCEquipment::Swing family;
        auto timing = SCEquipment::timing(kind);
        family.begin(17, kind);
        float previous = -1;
        int active = 0;
        for (int frame = 0; frame < timing.total; frame++) {
            assert(family.running() && family.age == frame);
            assert(family.phase() >= previous && family.phase() <= 1);
            previous = family.phase();
            if (family.active()) {
                active++;
                assert(family.phase() >= .1999f && family.phase() <= .5501f);
                assert(family.claim(42) == (frame == timing.first));
            } else
                assert(!family.claim(42));
            assert(!family.tick(true, true, 17));
        }
        assert(!family.running() && active == timing.last - timing.first + 1);
        family.tick(false, true, 17);
        assert(family.tick(true, true, 17));
        family.begin(17, kind);
        for (int j = 0; j < timing.first; j++)
            family.tick(false, true, 17);
        assert(family.claim(42));
        family.tick(false, true, 18);
        assert(!family.running());
    }
    SCEquipment::Swing s;
    assert(s.tick(true, true, 1));
    s.begin(1);
    assert(s.age == 0 && !s.active());
    for (int i = 0; i < 5; i++)
        assert(!s.tick(true, true, 1));
    assert(s.active());
    assert(s.claim(7));
    assert(!s.claim(7));
    for (int i = 5; i < 24; i++)
        assert(!s.tick(true, true, 1));
    assert(!s.running()); // Hold never repeats.
    assert(!s.tick(false, true, 1));
    assert(s.tick(true, true, 1));
    assert(s.intendedCombo() == 1);
    s.begin(1);
    assert(!s.tick(false, true, 2));
    assert(!s.running()); // Gear switch cannot carry a hit.
    s.reset();
    assert(s.tick(true, true, 3)); // Insufficient stamina: caller does not begin.
    assert(!s.tick(true, true, 3));
    assert(!s.running());
    assert(!s.tick(false, true, 3));
    assert(s.tick(true, true, 3));
    s.begin(3);
    assert(!s.tick(false, false, 3));
    assert(!s.running()); // Damage/air/interactions cancel.
    s.reset();
    s.begin(3);
    for (int i = 0; i < 13; i++)
        s.tick(false, true, 3);
    assert(!s.tick(true, true, 3));
    for (int i = 14; i < 23; i++)
        assert(!s.tick(false, true, 3));
    assert(s.tick(false, true, 3)); // One recovery buffer.
    SCContact::Capsule c = {{0, 0, 0}, .3f, 1.5f, .3f};
    SCEquipment::Point hit;
    assert(SCEquipment::contact({-1, 1, 0}, {1, 1, 0}, c, .05f, hit));
    assert(!SCEquipment::contact({-1, 2.5f, 0}, {1, 2.5f, 0}, c, .05f, hit));
    assert(SCEquipment::sweep({-1, 1, -1}, {1, 1, -1}, {-1, 1, 1}, {1, 1, 1}, c, .05f, hit));
    assert(!SCEquipment::sweep({-10, 1, 0}, {-9, 1, 0}, {-1, 1, 0}, {1, 1, 0}, c, .05f, hit));
    puts(
        "PASS equipment: commit/cancel, held input, combo buffer, exhaustion, gear change, native capsule sweep");
}
