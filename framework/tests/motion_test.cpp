#undef NDEBUG
#include "../core/motion.h"
#include "../core/session.h"
#include <cassert>
#include <cstdio>
#include <limits>

int main() {
    using namespace SCMotion;
    Mailbox box;
    Ticket ticket;
    assert(!box.publish({}, {1, 0, 0}, 0, 0));
    box.bind();
    assert(box.publish({}, {.3f, 0, 0}, 0, 100));
    assert(!box.publish({}, {.6f, 0, 0}, 0, 100));
    assert(box.consume(110, ticket));
    assert(!box.consume(111, ticket));
    assert(!box.ready());
    assert(box.acknowledge(ticket));
    assert(ticket.to.x == .3f && box.ready());
    assert(!box.acknowledge(ticket));
    assert(box.publish({}, {1, 0, 0}, 0, 200));
    auto epoch = box.generation();
    box.reset();
    assert(box.generation() != epoch && !box.consume(201, ticket));
    box.bind();
    assert(box.publish({}, {1, 0, 0}, 0, 300));
    assert(box.expired(551));
    assert(!box.consume(552, ticket));
    assert(box.ready());
    assert(!box.publish({}, {3, 0, 0}, 0, 600));
    assert(!box.publish({}, {0, std::numeric_limits<float>::quiet_NaN(), 0}, 0, 600));
    assert(box.publish({}, {1, 0, 0}, 0, 0xfffffff0));
    assert(box.consume(20, ticket));
    assert(box.acknowledge(ticket));
    for (int hz : {30, 60, 120}) {
        Clock clock;
        int ticks = 0;
        for (int i = 0; i < hz * 5; i++) {
            clock.advance(1. / hz);
            ticks += clock.take(true);
        }
        assert(ticks == 150);
    }
    Clock clock;
    clock.advance(10);
    assert(clock.take(true));
    assert(clock.take(true));
    assert(!clock.take(true));
    clock.clear();
    clock.advance(.1);
    assert(!clock.take(false));
    assert(clock.take(true));
    assert(!clock.take(true));
    InputLatch latch;
    SC_Input input{};
    input.attack = 1;
    latch.sample(input, 10);
    input.attack = 0;
    input.moveY = 1;
    latch.sample(input, 20);
    auto out = latch.take(25);
    assert(out.attack && out.moveY == 1);
    assert(!latch.take(26).attack);
    input.jump = 1;
    latch.sample(input, 30);
    assert(!latch.take(200).jump);
    auto v = clip({100, 300, 0}, {-1, 0, 0});
    assert(v.x == 0 && v.y == 300);
    v = clip({100, 300, 0}, {0, -1, 0});
    assert(v.y == 0 && v.x == 100);
    v = clip({100, -300, 0}, {0, 1, 0});
    assert(v.y == 0 && v.x == 100);
    NativeSupport support;
    Ticket falling{};
    falling.from = {0, 0, 0};
    falling.to = {0, -.08f, 0};
    support.observe(&falling, {0, 0, 0});
    assert(!support.supported());
    support.observe(&falling, {0, 0, 0});
    assert(support.supported());
    support.observe(nullptr, {.03f, 0, 0});
    assert(support.supported()); // steady platform
    support.observe(nullptr, {.03f, -.06f, 0});
    assert(!support.supported()); // pit
    support.observe(&falling, {.3f, 0, 0});
    assert(!support.supported()); // side correction is not floor
    support.observe(&falling, {0, 0, 0});
    assert(!support.supported());
    support.observe(&falling, {0, 0, 0});
    assert(support.supported());
    Ticket jump{};
    jump.from = {0, 0, 0};
    jump.to = {0, .06f, 0};
    support.observe(&jump, {0, .06f, 0});
    assert(!support.supported());
    support.observe(&falling, {0, 0, 0});
    support.observe(&falling, {0, 0, 0});
    assert(support.supported());
    support.reset();
    assert(!support.supported());
    SCSession::Session session;
    SCSession::Identity identity{1, 2, 3, 1801};
    session.request(true);
    assert(!session.canRetry(identity, true, 1000));
    assert(session.canRetry(identity, true, 1500));
    session.activate(identity);
    assert(session.matches(identity) && session.character());
    assert(session.handoff() && session.action());
    assert(!session.handoff());
    assert(session.resume() && session.character());
    session.release();
    assert(session.requested() && !session.visible());
    assert(!session.canRetry(identity, false, 2000));
    assert(session.canRetry(identity, true, 2500));
    assert(!session.canRetry({1, 2, 4, 1801}, true, 3000));
    assert(!session.matches(identity));
    session.request(false);
    assert(!session.canRetry(identity, true, 4000));
    assert(!session.die());
    session.activate(identity);
    assert(session.die() && session.death() && session.visible() && !session.character() &&
           !session.action());
    assert(!session.resume() && !session.handoff() && !session.die() &&
           !session.canRetry(identity, true, 5000));
    session.release();
    assert(session.requested() && !session.death());
    assert(!session.canRetry(identity, true, 6000));
    assert(session.canRetry(identity, true, 6500));
    session.activate(identity);
    assert(session.handoff() && session.die());
    assert(!session.matches({2, 2, 3, 1801}));
    session.release();
    assert(!session.visible());
    SCSession::QuietGate gate;
    assert(!gate.update(true, 0));
    assert(!gate.update(false, 99));
    assert(!gate.update(true, 100));
    assert(gate.update(true, 200));
    // Class activation must not require standing idle for 500 continuous ms.
    SCSession::Session loading;
    loading.request(true);
    assert(!loading.canRetry(identity, false, 100));
    assert(!loading.canRetry(identity, false, 600));       // loading/menu still owns input
    assert(loading.canRetry(identity, true, 601));         // first safe frame after stable load
    assert(!loading.canRetry({1, 2, 4, 1801}, true, 602)); // physics replacement still settles
    assert(!loading.canRetry({1, 2, 4, 1801}, false, 1102));
    assert(loading.canRetry({1, 2, 4, 1801}, true, 1103));
    loading.request(false);
    assert(!loading.canRetry(identity, true, 1600));
    // Repeated seamless Firelink/Burg crossings retain guest ownership. Actual
    // actor/physics replacement and scripted ownership still require a handoff.
    loading.activate({1, 2, 3, 1002});
    auto generation = loading.generation();
    assert(loading.changeMap({1, 2, 3, 1001}) && loading.character() &&
           loading.matches({1, 2, 3, 1001}));
    assert(loading.generation() == generation + 1);
    assert(loading.changeMap({1, 2, 3, 1002}) && loading.character());
    assert(!loading.changeMap({1, 4, 3, 1001}) && !loading.changeMap({4, 2, 3, 1001}) &&
           !loading.changeMap({1, 2, 3, -1}));
    assert(loading.handoff() && !loading.changeMap({1, 2, 3, 1001}));
    puts(
        "PASS motion ownership: single-use requests, backpressure, stale/epoch rejection, finite bounds, clock-rate parity, input latch, velocity clipping, actor lifecycle and timed interaction return");
}
