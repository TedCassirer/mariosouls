#undef NDEBUG
#include "../core/motion.h"
#include "../core/session.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main(){
 using namespace SCMotion;Mailbox box;Ticket ticket;assert(!box.publish({}, {1,0,0},0,0));box.bind();
 assert(box.publish({}, {.3f,0,0},0,100));assert(!box.publish({}, {.6f,0,0},0,100));assert(box.consume(110,ticket));assert(!box.consume(111,ticket));assert(!box.ready());assert(box.acknowledge(ticket));assert(ticket.to.x==.3f&&box.ready());assert(!box.acknowledge(ticket));
 assert(box.publish({}, {1,0,0},0,200));auto epoch=box.generation();box.reset();assert(box.generation()!=epoch&&!box.consume(201,ticket));box.bind();assert(box.publish({}, {1,0,0},0,300));assert(box.expired(551));assert(!box.consume(552,ticket));assert(box.ready());
 assert(!box.publish({}, {3,0,0},0,600));assert(!box.publish({}, {0,std::numeric_limits<float>::quiet_NaN(),0},0,600));
 assert(box.publish({}, {1,0,0},0,0xfffffff0));assert(box.consume(20,ticket));assert(box.acknowledge(ticket));
 for(int hz:{30,60,120}){Clock clock;int ticks=0;for(int i=0;i<hz*5;i++){clock.advance(1./hz);ticks+=clock.take(true);}assert(ticks==150);}
 Clock clock;clock.advance(10);assert(clock.take(true));assert(clock.take(true));assert(!clock.take(true));clock.clear();clock.advance(.1);assert(!clock.take(false));assert(clock.take(true));assert(!clock.take(true));
 InputLatch latch;SC_Input input{};input.attack=1;latch.sample(input,10);input.attack=0;input.moveY=1;latch.sample(input,20);auto out=latch.take(25);assert(out.attack&&out.moveY==1);assert(!latch.take(26).attack);input.jump=1;latch.sample(input,30);assert(!latch.take(200).jump);
 auto v=clip({100,300,0},{-1,0,0});assert(v.x==0&&v.y==300);v=clip({100,300,0},{0,-1,0});assert(v.y==0&&v.x==100);v=clip({100,-300,0},{0,1,0});assert(v.y==0&&v.x==100);
 SCSession::Session session;SCSession::Identity identity{1,2,3,1801};session.request(true);assert(!session.canRetry(identity,true,1000));assert(session.canRetry(identity,true,1500));session.activate(identity);assert(session.matches(identity)&&session.character());assert(session.handoff()&&session.action());assert(!session.handoff());assert(session.resume()&&session.character());session.release();assert(session.requested()&&!session.visible());assert(!session.canRetry(identity,false,2000));assert(!session.canRetry(identity,true,2500));assert(!session.canRetry({1,2,4,1801},true,3000));assert(!session.matches(identity));session.request(false);assert(!session.canRetry(identity,true,4000));
 SCSession::QuietGate gate;assert(!gate.update(true,0));assert(!gate.update(false,99));assert(!gate.update(true,100));assert(gate.update(true,200));
 puts("PASS motion ownership: single-use requests, backpressure, stale/epoch rejection, finite bounds, clock-rate parity, input latch, velocity clipping, actor lifecycle and timed interaction return");
}
