// Offline host fixture, not a game implementation. Reuses production policies.
#undef NDEBUG
#include "../core/session.h"
#include "../core/motion.h"
#include "../core/starting_class.h"
#include "../core/host_capabilities.h"
#include "../core/qa_identity.h"
#include <cassert>
#include <cstdio>
using SCSession::Identity;
using SCSession::Owner;

struct SimulatedHost {
    SCSession::Session session;
    SCClass::Selection selection;
    SCMotion::Mailbox motion;
    SCMotion::NativeSupport support;
    SCTiming::Clock clock;
    SCMotion::InputLatch input;
    SCHost::Capabilities capabilities{255};
    unsigned rigs = 0, terrainRefreshes = 0;

    void clearPending() {
        motion.reset();
        clock.clear();
        input.clear();
        support.reset();
    }

    void observe(Identity id, bool living, uint32_t now, bool safe = true, bool known = true) {
        auto intent = selection.observe(id.actor, known, true, id.map, living);
        if (intent == SCClass::Decision::Enable)
            session.request(true);
        if (intent == SCClass::Decision::Disable)
            session.request(false);
        auto change = SCSession::transition(session, id, true, true);
        if (change == SCSession::Transition::SeamlessMap) {
            assert(session.changeMap(id));
            ++terrainRefreshes;
            motion.bind();
        } else if (change == SCSession::Transition::ReplaceActor) {
            session.release();
            clearPending();
        }
        if (session.visible() && !living) {
            session.die();
            clearPending();
        }
        if (session.death() && living) {
            session.release();
            clearPending();
        }
        if (session.canRetry(
                id, safe && living && SCHost::permits(capabilities, SCHost::Operation::Movement),
                now)) {
            session.activate(id);
            motion.bind();
            ++rigs;
        }
    }
};

int main() {
    // Compare extracted classification to the former PTDE branch predicates.
    // Enumerate every owner and every identity component, including invalid reads.
    for (int owner = 0; owner < 4; ++owner) {
        SCSession::Session session;
        Identity initial{1, 2, 3, 10};
        if (owner)
            session.activate(initial);
        if (owner == 2)
            session.handoff();
        if (owner == 3)
            session.die();
        for (Identity next : {initial, Identity{1, 2, 3, 11}, Identity{4, 2, 3, 10},
                              Identity{1, 4, 3, 10}, Identity{1, 2, 4, 10}, Identity{}})
            for (bool camera : {false, true})
                for (bool allow : {false, true}) {
                    bool seamless = session.character() && allow && camera &&
                                    !session.matches(next) && session.sameActor(next);
                    bool replace = session.visible() && (!session.matches(next) || !camera);
                    auto expected = seamless  ? SCSession::Transition::SeamlessMap
                                    : replace ? SCSession::Transition::ReplaceActor
                                              : SCSession::Transition::Stable;
                    assert(SCSession::transition(session, next, camera, allow) == expected);
                }
    }
    SimulatedHost h;
    Identity a{1, 2, 3, 10};
    h.observe({}, false, 0);
    assert(!h.session.visible());
    h.observe(a, true, 100, false);
    h.observe(a, true, 599);
    assert(!h.session.visible());
    h.observe(a, true, 600);
    assert(h.session.character() && h.rigs == 1);
    auto epoch = h.session.generation();
    assert(h.motion.publish({}, {.1f, 0, 0}, 0, 610));
    a.map = 11;
    h.observe(a, true, 620, false, false);
    assert(h.session.character() && h.rigs == 1 && h.terrainRefreshes == 1);
    assert(h.session.generation() != epoch && h.motion.ready());
    assert(SCSession::transition(h.session, a, false, true) == SCSession::Transition::ReplaceActor);
    assert(h.motion.publish({}, {.1f, 0, 0}, 0, 630));
    SC_Input pressed{};
    pressed.attack = 1;
    h.input.sample(pressed, 630);
    assert(h.session.handoff());
    h.clearPending();
    SCMotion::Ticket stale;
    assert(!h.motion.consume(640, stale) && !h.input.take(640).attack);
    assert(h.session.action() && h.session.visible());
    SCSession::QuietGate gate;
    assert(!gate.update(true, 700));
    assert(!gate.update(false, 750));
    assert(!gate.update(true, 800));
    assert(gate.update(true, 900));
    assert(h.session.resume());
    h.motion.bind();
    assert(h.session.character() && h.rigs == 1);
    // Death releases pending movement; native HP respawn (even reused identity)
    // requires a fresh stable activation. Saved-class intent survives it.
    h.observe(a, false, 1000);
    assert(h.session.death() && !h.motion.busy());
    h.observe(a, true, 1100);
    assert(!h.session.visible() && h.session.requested());
    h.observe(a, true, 1600);
    assert(h.session.character() && h.rigs == 2);
    auto b = a;
    b.movement = 30;
    assert(h.motion.publish({}, {.1f, 0, 0}, 0, 1700));
    auto generation = h.motion.generation();
    h.observe(b, true, 1710);
    assert(!h.session.visible() && !h.motion.busy());
    assert(h.motion.generation() != generation);
    h.observe(b, true, 2210);
    assert(h.session.character() && h.rigs == 3);
    // Native moving platform observations, then removed support and a jump.
    SCMotion::Ticket down{1, 1, 0, {0, 1, 0}, {0, .9f, 0}, 0};
    h.support.observe(&down, {0, 1, 0});
    h.support.observe(&down, {0, 1.01f, 0});
    assert(h.support.supported());
    h.support.observe(nullptr, {0, 1.03f, 0});
    assert(h.support.supported());
    h.support.observe(nullptr, {0, .9f, 0});
    assert(!h.support.supported());
    SCMotion::Ticket up{1, 2, 0, {0, .9f, 0}, {0, 1, 0}, 0};
    h.support.observe(&up, {0, 1, 0});
    assert(!h.support.supported());
    for (auto op :
         {SCHost::Operation::Movement, SCHost::Operation::Combat, SCHost::Operation::Interaction,
          SCHost::Operation::Platform, SCHost::Operation::WorldDraw}) {
        assert(!SCHost::permits({}, op));
        assert(SCHost::permits({255}, op));
        auto required = SCHost::required(op);
        for (uint32_t bit = 1; bit <= 128; bit <<= 1)
            if (required & bit)
                assert(!SCHost::permits({255u & ~bit}, op));
    }
    h.session.release();
    h.capabilities = {0};
    h.observe(b, true, 3000);
    h.observe(b, true, 4000);
    assert(!h.session.visible());
    assert(!SCQA::meleeFixture(65, 11, 11, 11, 16, 13, 11, 11, 11));
    // Counter wrap and failed-activation cooldown are native-clock concerns.
    SCSession::Session wrap;
    wrap.request(true);
    assert(!wrap.canRetry(a, true, 0xffffff00u));
    assert(wrap.canRetry(a, true, 0x100u));
    wrap.failed(0x100u);
    assert(!wrap.canRetry(a, true, 0x500u));
    assert(!wrap.canRetry(a, true, 0x900u));
    assert(wrap.canRetry(a, true, 0xb00u));
    puts(
        "PASS simulated loading, death/respawn, seamless maps, replacement, platforms, native control, capabilities and test guard");
}
