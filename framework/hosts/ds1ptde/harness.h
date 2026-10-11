// Opt-in, bounded test transport. Normal gameplay does not open its event file.
// Included after the native interaction observer; all state runs on the update thread.
#include "harness_policy.h"
struct HarnessRuntime {
 bool armed=false,requirePad=false,suppressOutgoing=false;DWORD session=0,closedSession=0,command=0,heartbeat=0,started=0,lastPoll=0;
 uintptr_t owner=0,targetRef=0;int targetId=1810101,remaining=0,buttons=0,buttonDelay=0,prevHP=-1,prevPlayerHP=-1;
 float x=0,y=0,camX=0,camZ=1;unsigned frame=0,attack=0,attempt=0;uint32_t previousAction=0;FILE* events=nullptr;
 char config[MAX_PATH]={};
} harness;
static bool harnessIdentity(){
 uintptr_t stats=chain(0x1378700,{0,8});wchar_t name[20]={};SIZE_T got=0;
 if(!stats||!ReadProcessMemory(GetCurrentProcess(),(void*)(stats+0xa0),name,sizeof(name),&got)||got!=sizeof(name))return false;
 name[19]=0;int level=read<int>(stats+0x88);
 return (!wcscmp(name,L"test")||!wcscmp(name,L"Mario QA"))&&level>0&&level<=10&&currentMap()==1801;
}
static uintptr_t harnessTarget(){
 uintptr_t root=read<uint32_t>(playerSlot),begin=read<uint32_t>(root+4),end=read<uint32_t>(root+8),result=0;
 if(!begin||end<begin||end-begin>16384)return 0;
 for(uintptr_t at=begin;at<end;at+=4){uintptr_t p=read<uint32_t>(at);if(p&&read<int>(p+0x208)==harness.targetId){if(result)return 0;result=p;}}
 return result;
}
static void harnessEvent(const char* type,const char* fields=""){
 if(!harness.events)return;
 fprintf(harness.events,"{\"type\":\"%s\",\"tick\":%lu,\"session\":%lu,\"command\":%lu,\"frame\":%u,\"attack_id\":%u,\"attempt_id\":%u%s}\n",type,GetTickCount(),harness.session,harness.command,harness.frame,harness.attack,harness.attempt,fields);fflush(harness.events);
}
static void harnessStop(const char* why,bool restore=true){
 if(!harness.armed)return;char fields[200];snprintf(fields,sizeof(fields),",\"reason\":\"%s\"",why);harnessEvent("stopped",fields);
 harness.armed=false;harness.remaining=0;harness.buttons=0;harness.x=harness.y=0;testFrames=0;testButtons=0;testX=testY=0;harness.closedSession=harness.session;
 if(restore&&(enabled||interacting))releaseControl();
 if(harness.events){fclose(harness.events);harness.events=nullptr;}
 logf("harness stopped session=%lu reason=%s",harness.session,why);
}
static void harnessObserve(bool focus,bool padConnected){
 if(!harness.armed)return;
 uintptr_t npc=harnessTarget();Vec p=read<Vec>(posData+0x10);uintptr_t np=chain(npc+0x28,{0,0x1c});Vec n=read<Vec>(np+0x10);
 int hp=npc?read<int>(npc+0x2d4):-1,php=read<int>(player+0x2d4);auto native=nativeInteractionState();
 char fields[1500];
 snprintf(fields,sizeof(fields),",\"player_ref\":%u,\"target_ref\":%u,\"target_id\":%d,\"map\":%d,\"player_hp\":%d,\"target_hp\":%d,\"stamina\":%d,\"player_pos\":[%.5f,%.5f,%.5f],\"target_pos\":[%.5f,%.5f,%.5f],\"player_facing\":%.5f,\"target_facing\":%.5f,\"action\":%u,\"animation\":%d,\"animation_frame\":%d,\"focus\":%d,\"controller\":%d,\"menu\":%d,\"interaction\":%d,\"native_idle\":%d,\"active\":%d,\"frames_remaining\":%d,\"flags1\":%u,\"flags2\":%u",
 (unsigned)player,(unsigned)npc,harness.targetId,currentMap(),php,hp,read<int>(player+0x2e4),p.x,p.y,p.z,n.x,n.y,n.z,read<float>(posData+4),read<float>(np+4),state.rawAction,state.rawAnimation,state.rawAnimationFrame,focus,padConnected,native.menu,interacting||native.event||native.ladder,native.idle,enabled,harness.remaining,read<uint32_t>(player+0x1fc),read<uint32_t>(player+0x3c4));
 harnessEvent("observation",fields);
 if(npc==harness.targetRef&&harness.prevHP>=0&&hp>=0&&hp!=harness.prevHP){snprintf(fields,sizeof(fields),",\"target_id\":%d,\"before\":%d,\"after\":%d,\"damage\":%d",harness.targetId,harness.prevHP,hp,harness.prevHP-hp);harnessEvent(hp<harness.prevHP?"confirmed_target_hp_loss":"target_heal",fields);}
 if(harness.prevPlayerHP>=0&&php!=harness.prevPlayerHP){snprintf(fields,sizeof(fields),",\"before\":%d,\"after\":%d,\"damage\":%d",harness.prevPlayerHP,php,harness.prevPlayerHP-php);harnessEvent(php<harness.prevPlayerHP?"confirmed_player_hp_loss":"player_heal",fields);}
 harness.prevHP=hp;harness.prevPlayerHP=php;
}
static void harnessTick(bool focus,bool padConnected){
 DWORD now=GetTickCount();
 if(harness.armed&&(!harnessIdentity()||!player||player!=harness.owner||!posData||!cam)){harnessStop("character_map_or_loading_changed");return;}
 if(harness.armed&&!focus){harnessStop("focus_lost");return;}
 if(!harness.config[0])snprintf(harness.config,sizeof(harness.config),"%s\\MarioSouls\\harness.ini",root);
 if(DWORD(now-harness.lastPoll)>=50){
  harness.lastPoll=now;DWORD session=GetPrivateProfileIntA("Harness","Session",0,harness.config),serial=GetPrivateProfileIntA("Harness","Serial",0,harness.config);
  DWORD heartbeat=GetPrivateProfileIntA("Harness","Heartbeat",0,harness.config);int armed=GetPrivateProfileIntA("Harness","Armed",0,harness.config);
  if(harness.armed&&(session!=harness.session||!armed)){
   char reason[40];GetPrivateProfileStringA("Harness","StopReason","cancelled",reason,sizeof(reason),harness.config);
   harnessStop(session==harness.session&&strcmp(reason,"focus_lost")==0?"focus_lost":"cancelled");return;
  }
  if(!harness.armed&&armed&&session&&session!=harness.closedSession&&DWORD(now-heartbeat)<2000){
   harness.session=session;harness.command=serial;harness.closedSession=session;
   if(!focus||!harnessIdentity()||!player||read<int>(player+0x2d4)<=0){logf("harness arm rejected: focus/identity/living-player guard");return;}
   auto native=nativeInteractionState();if(!native.valid||native.menu||native.event||native.ladder||interacting){logf("harness arm rejected: native busy");return;}
   harness.targetId=GetPrivateProfileIntA("Harness","Target",1810101,harness.config);uintptr_t target=harnessTarget();
   if(!target||read<int>(target+0x2d4)<=0){uintptr_t rr=read<uint32_t>(playerSlot),bb=read<uint32_t>(rr+4),ee=read<uint32_t>(rr+8);logf("harness arm rejected: target absent/dead id=%d target=%08x hp=%d slot=%08x root=%08x begin=%08x end=%08x",harness.targetId,(unsigned)target,read<int>(target+0x2d4),(unsigned)playerSlot,(unsigned)rr,(unsigned)bb,(unsigned)ee);if(bb&&ee>=bb&&ee-bb<=16384)for(uintptr_t at=bb;at<ee;at+=4){uintptr_t pp=read<uint32_t>(at);if(pp)logf("harness candidate ref=%08x id=%d hp=%d",(unsigned)pp,read<int>(pp+0x208),read<int>(pp+0x2d4));}return;}
   char path[MAX_PATH];snprintf(path,sizeof(path),"%s\\MarioSouls\\harness-events.jsonl",root);harness.events=fopen(path,"a");if(!harness.events){logf("harness arm rejected: cannot open report");return;}
   harness.armed=true;harness.started=now;harness.heartbeat=heartbeat;harness.owner=player;harness.targetRef=target;harness.remaining=0;harness.frame=harness.attack=harness.attempt=0;harness.previousAction=state.rawAction;harness.prevHP=-1;harness.prevPlayerHP=-1;testFrames=0;testButtons=0;
   float heading=read<float>(posData+4);harness.camX=-sinf(heading);harness.camZ=-cosf(heading);
   harness.suppressOutgoing=GetPrivateProfileIntA("Harness","SuppressOutgoing",0,harness.config)!=0;
   harness.requirePad=GetPrivateProfileIntA("Harness","RequireController",padConnected?1:0,harness.config)!=0;harness.command=0;harnessEvent("armed",harness.suppressOutgoing?",\"suppress_outgoing\":true":",\"suppress_outgoing\":false");
  }
  if(harness.armed){
   harness.heartbeat=heartbeat;
   if(serial!=harness.command){
    harness.command=serial;int mode=GetPrivateProfileIntA("Harness","Mode",0,harness.config);
    harness.remaining=0;harness.buttons=0;harness.buttonDelay=0;harness.x=harness.y=0;
    if(mode==4){harnessStop("cancelled");return;}
    if(mode==1){if(enabled||interacting)releaseControl();if(!activate()){harnessStop("unsafe_activation");return;}}
    else if(mode==2){if(enabled||interacting)releaseControl();}
    else if(mode==5){int height=GetPrivateProfileIntA("Harness","Frames",0,harness.config);if(!combatTestDrop(height)){harnessStop("invalid_test_drop");return;}}
    else if(mode==3){
     int frames=GetPrivateProfileIntA("Harness","Frames",0,harness.config),buttons=GetPrivateProfileIntA("Harness","Buttons",0,harness.config);char sx[40],sy[40];GetPrivateProfileStringA("Harness","X","0",sx,sizeof(sx),harness.config);GetPrivateProfileStringA("Harness","Y","0",sy,sizeof(sy),harness.config);float x=strtof(sx,nullptr),y=strtof(sy,nullptr);
     harness.buttonDelay=GetPrivateProfileIntA("Harness","ButtonDelay",0,harness.config);
     if(!enabled||!HarnessPolicy::validInput(frames,buttons,x,y)||harness.buttonDelay<0||harness.buttonDelay>=frames){harnessStop("invalid_input_command");return;}
     harness.remaining=frames;harness.buttons=buttons;harness.x=x;harness.y=y;
    }else if(mode!=0){harnessStop("invalid_command");return;}
    char fields[200];snprintf(fields,sizeof(fields),",\"mode\":%d,\"frames\":%d,\"buttons\":%d,\"x\":%.4f,\"y\":%.4f",mode,harness.remaining,harness.buttons,harness.x,harness.y);harnessEvent("command_accepted",fields);
   }
  }
 }
 if(!harness.armed)return;
 if(!harnessIdentity()||!player||player!=harness.owner||!posData||!cam){harnessStop("character_map_or_loading_changed");return;}
 harnessObserve(focus,padConnected);
 auto native=nativeInteractionState();HarnessPolicy::State guard;guard.focus=focus;guard.alive=read<int>(player+0x2d4)>0;guard.target=harnessTarget()==harness.targetRef;guard.busy=!native.valid||native.menu||native.event||native.ladder||interacting;guard.controller=padConnected;guard.requireController=harness.requirePad;guard.now=now;guard.heartbeat=harness.heartbeat;guard.started=harness.started;
 if(auto why=HarnessPolicy::stopReason(guard)){
  // Native damage can land between the observation and the guard read.
  // Capture terminal HP before closing the journal on death.
  if(strcmp(why,"player_died")==0)harnessObserve(focus,padConnected);
  harnessStop(why);return;
 }
 if((read<uint32_t>(player+0x1fc)&0x0e000000)||(read<uint32_t>(player+0x3c4)&0x4e0)){harnessStop("player_protection_enabled");return;}
}
static void harnessInput(SC_Input& input){
 if(!harness.armed)return;input.camLookX=harness.camX;input.camLookZ=harness.camZ;input.moveX=input.moveY=0;input.jump=input.attack=input.crouch=0;
 if(harness.remaining>0){input.moveX=-harness.x;input.moveY=-harness.y;int b=harness.buttonDelay>0?0:harness.buttons;if(harness.buttonDelay>0)--harness.buttonDelay;input.jump=!!(b&1);input.attack=!!(b&2);input.crouch=!!(b&4);--harness.remaining;}
}
static void harnessAfterStep(uint32_t old,const SC_Input& input){
 if(!harness.armed)return;++harness.frame;char fields[400];
 snprintf(fields,sizeof(fields),",\"x\":%.4f,\"y\":%.4f,\"buttons\":%d,\"action\":%u,\"animation\":%d,\"animation_frame\":%d,\"speed\":%.3f,\"vertical_speed\":%.3f,\"action_arg\":%u,\"action_timer\":%u,\"stamina\":%d",-input.moveX,-input.moveY,(input.jump?1:0)|(input.attack?2:0)|(input.crouch?4:0),state.rawAction,state.rawAnimation,state.rawAnimationFrame,(state.forwardSpeed/SC_FIXED_HZ),state.velocity[1]/SC_FIXED_HZ,state.rawActionArg,state.rawActionTimer,read<int>(player+0x2e4));harnessEvent("input_frame",fields);
 if(old!=state.rawAction){snprintf(fields,sizeof(fields),",\"from\":%u,\"to\":%u,\"animation\":%d",old,state.rawAction,state.rawAnimation);harnessEvent("action_changed",fields);}
 static int priorAttack=-1;int nextAttack=state.attackId;
 if(priorAttack>=0&&priorAttack!=nextAttack)harnessEvent("attack_end");
 if(nextAttack>=0&&priorAttack!=nextAttack){++harness.attack;harnessEvent("attack_start");}
 priorAttack=nextAttack;
 if(harness.remaining==0&&harness.buttons>=0){harnessEvent("command_frames_complete");harness.buttons=-1;}
}
static void harnessHitAttempt(int id,int hp,int behavior,int result,const char* type){
 if(!harness.armed||id!=harness.targetId)return;if(strcmp(type,"hit_attempt")==0)++harness.attempt;
 char fields[300];snprintf(fields,sizeof(fields),",\"target_id\":%d,\"hp_before\":%d,\"behavior\":%d,\"native_return\":%d,\"action\":%u,\"animation\":%d",id,hp,behavior,result,state.rawAction,state.rawAnimation);harnessEvent(type,fields);
}
