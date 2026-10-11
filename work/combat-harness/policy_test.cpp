#include "../harness_policy.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
using namespace HarnessPolicy;
int main(){
 State s;assert(!stopReason(s));s.identity=false;assert(!strcmp(stopReason(s),"character_map_or_loading_changed"));
 s={};s.focus=false;assert(!strcmp(stopReason(s),"focus_lost"));s={};s.controller=false;assert(!stopReason(s));s.requireController=true;assert(!strcmp(stopReason(s),"controller_disconnected"));
 s={};s.now=2001;assert(!strcmp(stopReason(s),"lease_expired"));s={};s.now=120001;s.heartbeat=120001;assert(!strcmp(stopReason(s),"lease_expired"));
 s={};s.started=s.heartbeat=0xffffff00;s.now=0x200;assert(!stopReason(s));s.now=0x1000;assert(!strcmp(stopReason(s),"lease_expired"));
 s={};s.alive=false;assert(!strcmp(stopReason(s),"player_died"));s={};s.target=false;assert(!strcmp(stopReason(s),"target_reference_changed"));s={};s.busy=true;assert(!strcmp(stopReason(s),"menu_or_interaction"));
 assert(validInput(1,0,0,0));assert(validInput(150,7,-1,1));assert(!validInput(151,0,0,0));assert(!validInput(0,0,0,0));assert(!validInput(-1,0,0,0));assert(!validInput(10,8,0,0));assert(!validInput(10,0,NAN,0));assert(!validInput(10,0,0,INFINITY));
 puts("PASS: character/map/loading, focus, controller loss, lease expiry/wrap, death, target replacement, menus, malformed and bounded frame inputs.");
}
