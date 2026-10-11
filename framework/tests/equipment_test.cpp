#undef NDEBUG
#include "../core/equipment.h"
#include <cassert>
#include <cstdio>
int main(){
 SCEquipment::Swing s;assert(s.tick(true,true,1));s.begin(1);assert(s.age==0&&!s.active());
 for(int i=0;i<5;i++)assert(!s.tick(true,true,1));assert(s.active());assert(s.claim(7));assert(!s.claim(7));
 for(int i=5;i<24;i++)assert(!s.tick(true,true,1));assert(!s.running()); // Hold never repeats.
 assert(!s.tick(false,true,1));assert(s.tick(true,true,1));assert(s.intendedCombo()==1);s.begin(1);
 assert(!s.tick(false,true,2));assert(!s.running()); // Gear switch cannot carry a hit.
 s.reset();assert(s.tick(true,true,3)); // Insufficient stamina: caller does not begin.
 assert(!s.tick(true,true,3));assert(!s.running());
 assert(!s.tick(false,true,3));assert(s.tick(true,true,3));s.begin(3);
 assert(!s.tick(false,false,3));assert(!s.running()); // Damage/air/interactions cancel.
 s.reset();s.begin(3);for(int i=0;i<13;i++)s.tick(false,true,3);assert(!s.tick(true,true,3));
 for(int i=14;i<23;i++)assert(!s.tick(false,true,3));assert(s.tick(false,true,3)); // One recovery buffer.
 SCContact::Capsule c={{0,0,0},.3f,1.5f,.3f};SCEquipment::Point hit;
 assert(SCEquipment::contact({-1,1,0},{1,1,0},c,.05f,hit));
 assert(!SCEquipment::contact({-1,2.5f,0},{1,2.5f,0},c,.05f,hit));
 assert(SCEquipment::sweep({-1,1,-1},{1,1,-1},{-1,1,1},{1,1,1},c,.05f,hit));
 assert(!SCEquipment::sweep({-10,1,0},{-9,1,0},{-1,1,0},{1,1,0},c,.05f,hit));
 puts("PASS equipment: commit/cancel, held input, combo buffer, exhaustion, gear change, native capsule sweep");
}
