#include "../mario_combat_policy.h"
#include <cassert>
using namespace MarioCombat;
int main(){
 assert(!pending(0,2364051890u));assert(pending(2364052390u,2364051890u));assert(!pending(2364052390u,2364052391u));assert(pending(50u,0xfffffff0u));
 assert(fallDamage(6,594)==0);assert(fallDamage(8,594)==0);assert(fallDamage(12,594)==99);assert(fallDamage(20,594)==594);
 Settings s;assert(power(s,10,10)==100);s.upgrade=15;assert(power(s,40,40)==355);assert(power(s,99,99)==399.25f);
 s.fixedPower=100;assert(power(s,99,99)==100);
 Window w;assert(w.observe(0x00800380,0,true));assert(w.pay(95,8,65));assert(!w.pay(87,8,65));assert(w.canHit(42));w.hit.insert(42);assert(!w.canHit(42));
 assert(!w.observe(0x00800457,2,true));assert(w.observe(0x00800457,3,true));assert(w.pay(87,8,65));assert(w.canHit(42));
 assert(w.observe(0x00800457,6,true));assert(w.pay(79,16,110));assert(w.active(0,0x200000,4,0));
 w.reset();w.observe(0x03000880,0,false);assert(w.beginStomp());assert(!w.pay(15,16,90));assert(w.exhausted);assert(!w.canHit(42));assert(!w.beginStomp());
 w.observe(0x03000880,0,false);assert(!w.beginStomp());w.observe(0x0c400201,0,true);assert(w.beginStomp());
 w.reset();assert(w.observe(0x0188088a,0,false));assert(w.pay(95,24,115));assert(!w.observe(0x00880456,0,true));assert(w.charged==24);
 w.reset();assert(w.observe(0x018008aa,0,false));assert(!w.observe(0x0080045a,0,true));
 w.reset();assert(w.observe(0x008008a9,0,false));assert(w.pay(95,30,165));assert(!w.observe(0x0080023c,0,true));for(int i=0;i<3;i++)w.hit.insert(i);assert(!w.canHit(4));
}
