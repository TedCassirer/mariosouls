#include "interaction_state.h"
#include <assert.h>
#include <stdio.h>
using namespace MarioInteraction;
int main(){
 ReturnGate gate;Observation ready={true,false,false,false,true,true,-1};
 gate.begin(100);assert(!gate.update(ready,true,100));assert(!gate.update(ready,true,1200));assert(gate.update(ready,true,1300));
 gate.begin(0);auto ladder=ready;ladder.ladder=true;ladder.idle=false;ladder.animation=7040;
 assert(!gate.update(ladder,true,100));assert(!gate.update(ladder,true,600000)); // No timeout mid-ladder.
 assert(!gate.update(ready,true,600001));assert(gate.update(ready,true,600351));
 gate.begin(0);auto bonfire=ready;bonfire.menu=true;bonfire.event=true;bonfire.animation=7721;
 assert(!gate.update(bonfire,true,10));assert(!gate.update(bonfire,true,100000));
 auto stand=ready;stand.event=true;assert(!gate.update(stand,true,100100));
 assert(!gate.update(ready,true,100400));assert(gate.update(ready,true,100750));
 gate.begin(0);auto unsafe=ready;unsafe.grounded=false;
 assert(!gate.update(unsafe,true,5000));assert(!gate.update(ready,false,6000));
 assert(!gate.update(ready,true,7000));assert(gate.update(ready,true,7350));
 gate.begin(0xffffff00u);assert(!gate.update(ready,true,0xffffff00u));assert(gate.update(ready,true,0x600u));
 puts("PASS: rejected interaction, ladder pause, bonfire menu/stand-up, unsafe floor, focus loss, clock wrap.");
}
