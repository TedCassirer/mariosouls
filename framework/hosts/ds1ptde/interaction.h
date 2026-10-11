#pragma once
#include <stdint.h>
namespace HostInteraction {
// Track 14 uses weapon/stance animation banks. In particular the observed
// two-handed 120000 idle must not be rejected as an active action.
inline bool idleAnimation(int id,uint32_t token){
    return token!=0xffffffffu&&id>=0&&(id%100000==0||id%100000==20000);
}
struct Observation {
    bool valid=false, menu=false, ladder=false, event=false, idle=false, grounded=false;
    int animation=-1;
};
class ReturnGate {
    uint32_t started=0,stableSince=0;
    bool stable=false,sawInteraction=false;
public:
    void begin(uint32_t now){started=now;stableSince=now;stable=false;sawInteraction=false;}
    bool update(const Observation& o,bool focus,uint32_t now) {
        if(o.menu||o.ladder||o.event)sawInteraction=true;
        if(!focus||!o.valid||o.menu||o.ladder||o.event||!o.idle||!o.grounded){stable=false;return false;}
        if(!stable){stable=true;stableSince=now;}
        // A short settling interval follows observed completion. An unanswered
        // interaction gets a longer grace period, but never overrides a busy state.
        return uint32_t(now-started)>=(sawInteraction?400u:1200u)&&uint32_t(now-stableSince)>=350u;
    }
};
}
