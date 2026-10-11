#pragma once
#include "mario_flask.h"
#include "mario_action_effects.h"
static DWORD actionRequested=0,actionPhaseStart=0;
static int actionRequestKind=0,actionNativeAnimation=-1;
static float actionLadderFrame=0;
static unsigned actionQuietFrames=0;
static int actionPendingHurt=0;static DWORD actionHurtTime=0;
static void actionReset(){actionRequestKind=0;actionQuietFrames=0;actionPendingHurt=0;actionCancelPulse();}
static bool actionPending(){return actionRequestKind&&DWORD(GetTickCount()-actionRequested)<600;}
static void actionFlags(bool nativeMotion){
 if(!player||player!=lastPlayer)return;
 auto f1=read<uint32_t>(player+0x1fc),f2=read<uint32_t>(player+0x3c4);
 put(player+0x1fc,nativeMotion?(f1&~0x4000u)|(savedFlags1&0x4000u):f1|0x4000u);
 bool allowAction=nativeMotion||actionPending();
 // Input filtering blocks native locomotion. Do not set NoMove: the native
 // interaction evaluator needs its ordinary movement update to refresh prompts.
 put(player+0x3c4,(f2&~0x300u)|(savedFlags2&0x300u)|(allowAction?0:0x100u));
}
static void requestAction(int kind,bool fromPad=false){
 if(!enabled||interacting||mario<0||!actionInputReady)return;
 auto o=nativeInteractionState();
 if(!o.valid||!o.grounded||(state.action&0x800)||!o.idle)return;
 if(kind==1&&!read<uint32_t>(read<uint32_t>(0x13786d0)+0x58)){logf("Action request rejected: no native prompt; Mario retained");return;}
 if(actionPending())return;
 actionRequested=GetTickCount();actionRequestKind=kind;actionFlags(false);actionQueue(kind,actionRequested,fromPad);
 logf("Action request kind=%d Mario=%d remains active",kind,mario);
}
static void beginInteraction(int slot,bool pressAction=true){
 if(pressAction){requestAction(1);return;}
 if(!enabled||interacting||mario<0)return;
 interacting=true;interactionPrevious=read<Vec>(posData+0x10);interactionMap=currentMap();
 actionNativeAnimation=-1;actionPhaseStart=GetTickCount();actionQuietFrames=0;actionLadderFrame=0;
 actionRequestKind=0;actionPendingHurt=0;actionCancelPulse();combatReset();worldActivate();
 actionFlags(true);lastHP=read<int>(player+0x2d4);
 logf("Action confirmed Mario=%d; native script owns action only",mario);
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
 if(changed){actionNativeAnimation=o.animation;actionPhaseStart=now;logf("Action phase native=%d menu=%d ladder=%d Mario=%d",o.animation,o.menu,o.ladder,mario);}
 float t=float(DWORD(now-actionPhaseStart))/1000.f;
 int animation=0xc3,style=0;float phase=-1;
 if(o.ladder){animation=0x05;actionLadderFrame+=delta.y*90.f;phase=actionLadderFrame;style=2;}
 else if(o.animation>=7585&&o.animation<=7587){animation=0x30;style=4;phase=o.animation==7585?std::min(1.f,t/.65f):o.animation==7586?1.f:std::max(0.f,1.f-t/.75f);}
 else if(o.animation>=7700&&o.animation<7750){
  int part=o.animation%10;
  animation=0x84;style=1;phase=part==0?std::min(1.f,t/1.1f):part==2?std::max(0.f,1.f-t/1.1f):1.f;
 }
 else if(o.animation==7698){animation=0x19;}
 else if(o.animation==7410){animation=0x48;}
 else if(o.animation>=7100&&o.animation<7400){animation=0x60;style=1;phase=std::min(1.f,t/2.2f);}
 // Unknown scripted actions retain a neutral Mario pose, never a generic looping door push.
 for(int step=0;acc>=1./30&&step<3;step++,acc-=1./30){
  Vec local=(host-origin)*100;
  if(fabsf(local.x)>4000||fabsf(local.y)>8000||fabsf(local.z)>4000){origin=host;rebuildSurfaces();local={0,0,0};}
  sm_host_pose(mario,local.x,local.y,local.z,read<float>(posData+4)-3.14159265f,animation,phase,style,&state,&geo);
  if(style==4&&phase>.02f)actionFlask();
  pumpAudio();
 }
 // Completion follows actual action/menu state, not the native walk/idle animation.
 if(!o.menu&&!o.event&&!o.ladder&&focus){++actionQuietFrames;}else actionQuietFrames=0;
 if(actionQuietFrames>=2){
  Vec local=(host-origin)*100;sm_resume(mario,local.x,local.y,local.z,read<float>(posData+4)-3.14159265f);
  if(actionPendingHurt&&DWORD(now-actionHurtTime)<700)sm_host_health(mario,actionPendingHurt);
  actionPendingHurt=0;
  state.position[0]=local.x;state.position[1]=local.y;state.position[2]=local.z;
  state.faceAngle=read<float>(posData+4)-3.14159265f;state.action=o.grounded?0x0c400201:0x0100088c;
  interacting=false;actionRequestKind=0;lastHP=read<int>(player+0x2d4);worldActivate();actionFlags(false);InterlockedExchange(&actionRoute,LONG(ActionRoute::Mario));acc=0;
  logf("Action complete; same Mario=%d resumes immediately",mario);
 }
}
