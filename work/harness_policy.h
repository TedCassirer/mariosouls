#pragma once
#include <stdint.h>
#include <math.h>
namespace HarnessPolicy {
struct State {bool identity=true,focus=true,alive=true,target=true,busy=false,controller=true,requireController=false;uint32_t now=0,heartbeat=0,started=0;};
inline const char* stopReason(const State& s){
 if(!s.identity)return "character_map_or_loading_changed";
 if(!s.focus)return "focus_lost";
 if(s.requireController&&!s.controller)return "controller_disconnected";
 if(uint32_t(s.now-s.heartbeat)>2000||uint32_t(s.now-s.started)>120000)return "lease_expired";
 if(!s.alive)return "player_died";
 if(!s.target)return "target_reference_changed";
 if(s.busy)return "menu_or_interaction";
 return nullptr;
}
inline bool validInput(int frames,int buttons,float x,float y){return frames>=1&&frames<=150&&buttons>=0&&buttons<=7&&isfinite(x)&&isfinite(y)&&fabsf(x)<=1&&fabsf(y)<=1;}
}
