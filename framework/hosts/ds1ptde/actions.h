#pragma once
#include "effects.h"
static DWORD actionRequested=0,actionPhaseStart=0;
static int actionRequestKind=0,actionNativeAnimation=-1;
static float actionLadderFrame=0;
static SCSession::QuietGate actionReturnGate;
static int actionPendingHurt=0;static DWORD actionHurtTime=0;
static void actionReset(){actionRequestKind=0;actionReturnGate.clear();actionPendingHurt=0;actionCancelPulse();}
static bool actionPending(){return actionRequestKind&&DWORD(GetTickCount()-actionRequested)<600;}
static void actionFlags(bool nativeMotion){
 if(!player||player!=lastPlayer)return;
 auto f1=read<uint32_t>(player+0x1fc),f2=read<uint32_t>(player+0x3c4);
 put(player+0x1fc,nativeMotion?(f1&~0x4000u)|(savedFlags1&0x4000u):f1|0x4000u);
 bool allowAction=nativeMotion||actionPending()||InterlockedCompareExchange(&equipmentGuardInput,0,0);
 // Input filtering blocks native locomotion. Do not set NoMove: the native
 // interaction evaluator needs its ordinary movement update to refresh prompts.
 put(player+0x3c4,(f2&~0x300u)|(savedFlags2&0x300u)|(allowAction?0:0x100u));
}
static bool actionEligible(int kind){
 if(!characterOwned()||nativeActionOwned()||nativeReaction||equipmentActionOwned||character<0||!actionInputReady||actionPending())return false;
 auto o=nativeInteractionState();
 if(!o.valid||!o.grounded||!(state.flags&SC_CAN_INTERACT)||!o.idle||o.menu||o.event||o.ladder)return false;
 return kind!=1||read<uint32_t>(read<uint32_t>(0x13786d0)+0x58)!=0;
}
static void requestAction(int kind,bool fromPad=false){
 if(!actionEligible(kind)){logf("Action request rejected: kind=%d not eligible",kind);return;}
 characterInput.clear();
 actionRequested=GetTickCount();actionRequestKind=kind;actionFlags(false);actionQueue(kind,actionRequested,fromPad);
 logf("Action request kind=%d character=%d remains active",kind,character);
}
static void beginInteraction(int slot,bool pressAction=true){
 if(pressAction){requestAction(1);return;}
 if(!characterOwned()||nativeActionOwned()||character<0)return;
 if(!characterSession.handoff())return;worldRelease();characterClock.clear();characterInput.clear();actionReturnGate.clear();interactionPrevious=read<Vec>(posData+0x10);interactionMap=currentMap();
 actionNativeAnimation=-1;actionPhaseStart=GetTickCount();actionLadderFrame=0;
 actionRequestKind=0;actionPendingHurt=0;actionCancelPulse();combatReset();
 actionFlags(true);lastHP=read<int>(player+0x2d4);
 logf("Action confirmed character=%d; native script owns action only",character);
}
static void updateInteraction(double& acc,bool focus,int slot){
 Vec host=read<Vec>(posData+0x10);auto o=nativeInteractionState();
 if(!o.valid||!player||player!=lastPlayer||!posData||!cam||currentMap()!=interactionMap||read<int>(player+0x2d4)<=0||!isfinite(host.y)||dot(host-interactionPrevious,host-interactionPrevious)>64){releaseControl();acc=0;return;}
 if(!focus)actionCancelPulse();
 int hp=read<int>(player+0x2d4);
 if(hp<lastHP){actionPendingHurt=lastHP-hp>=read<int>(player+0x2d8)/5?2:1;actionHurtTime=GetTickCount();logf("Action interrupted by native damage %d -> %d",lastHP,hp);}
 lastHP=hp;
 auto route=o.ladder?ActionRoute::Ladder:o.menu?ActionRoute::Menu:ActionRoute::Locked;
 InterlockedExchange(&actionRoute,LONG(route));actionFlags(true);
 if(hideNative){put(player+0x258,0.f);put(player+0x1fc,read<uint32_t>(player+0x1fc)&~0x800000u);}
 Vec delta=host-interactionPrevious;interactionPrevious=host;
 DWORD now=GetTickCount();bool changed=o.animation!=actionNativeAnimation;
 if(changed){actionNativeAnimation=o.animation;actionPhaseStart=now;logf("Action phase native=%d menu=%d ladder=%d character=%d",o.animation,o.menu,o.ladder,character);}
 float t=float(DWORD(now-actionPhaseStart))/1000.f;
 SC_Pose pose={};pose.kind=SC_POSE_NEUTRAL;pose.phase=-1;
 if(o.ladder){pose.kind=SC_POSE_CLIMB;actionLadderFrame+=delta.y*100;pose.travelCm=actionLadderFrame;}
 else if(o.animation>=7585&&o.animation<=7587){pose.kind=SC_POSE_DRINK;pose.phase=o.animation==7585?std::min(1.f,t/.65f):o.animation==7586?1.f:std::max(0.f,1.f-t/.75f);}
 else if(o.animation>=7700&&o.animation<7750){int part=o.animation%10;pose.kind=SC_POSE_REST;pose.phase=part==0?std::min(1.f,t/1.1f):part==2?std::max(0.f,1.f-t/1.1f):1.f;}
 else if(o.animation==7698)pose.kind=SC_POSE_DOWNED;
 else if(o.animation==7410)pose.kind=SC_POSE_WALK;
 else if(o.animation>=7100&&o.animation<7400){pose.kind=SC_POSE_DOOR;pose.phase=std::min(1.f,t/2.2f);}
 for(int step=0;acc>=1./SC_FIXED_HZ&&step<3;step++,acc-=1./SC_FIXED_HZ){
  Vec local=(host-origin)*100;
  if(fabsf(local.x)>4000||fabsf(local.y)>8000||fabsf(local.z)>4000){origin=host;rebuildSurfaces();local={0,0,0};}
  pose.position[0]=local.x;pose.position[1]=local.y;pose.position[2]=local.z;pose.yaw=read<float>(posData+4)-3.14159265f;
  if(!guestPose(character,&pose,&state,&geo)){releaseControl();acc=0;return;}
  pumpAudio();
 }
 // Completion follows actual action/menu state, not the native walk/idle animation.
 if(actionReturnGate.update(!o.menu&&!o.event&&!o.ladder&&o.idle&&focus,now)){
  Vec local=(host-origin)*100;guest->resume(character,local.x,local.y,local.z,read<float>(posData+4)-3.14159265f);
  if(actionPendingHurt&&DWORD(now-actionHurtTime)<700)pendingNativeHurt=std::max(pendingNativeHurt,uint32_t(actionPendingHurt));
  actionPendingHurt=0;
  state.position[0]=local.x;state.position[1]=local.y;state.position[2]=local.z;
  state.faceAngle=read<float>(posData+4)-3.14159265f;state.flags=o.grounded?(SC_GROUNDED|SC_CAN_INTERACT):0;state.attackId=state.interruptedAttackId=-1;
  characterSession.resume();actionRequestKind=0;lastHP=read<int>(player+0x2d4);worldActivate();characterClock.clear();characterInput.clear();actionFlags(false);InterlockedExchange(&actionRoute,LONG(ActionRoute::Character));acc=0;
  logf("Action complete; same character=%d resumes immediately",character);
 }
}
