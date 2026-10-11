#pragma once
// Native passive states (including guard break and damage reactions) retain
// ownership of motion. The guest supplies only its visible reaction pose.
static SCSession::QuietGate nativeReactionReturnGate;
static Vec nativeReactionPrevious;
static DWORD nativeReactionStarted=0;
static int nativeReactionMap=-1,nativeReactionStartHP=0,nativeReactionStartStamina=0;

static int nativeActiveAllowed(uintptr_t actor=player,bool captured=true){
 static int verified=0;
 if(!verified){
  // Chr ESD command100 / FDF1BB calls E2AEC0. Its bool assignment is at
  // E2AF34; E323B0 independently obtains the controller through movement+2c.
  const unsigned char setter[]={0x88,0x5e,0x3c,0x5e,0x5b,0xc2,0x04,0x00};
  const unsigned char access[]={0x56,0x8b,0xf1,0x8b,0x46,0x2c,0x85,0xc0};
  unsigned char setBytes[sizeof(setter)]={},accessBytes[sizeof(access)]={};
  verified=get(uintptr_t(0xe2af34),setBytes)&&!memcmp(setBytes,setter,sizeof(setter))&&
   get(uintptr_t(0xe323b0),accessBytes)&&!memcmp(accessBytes,access,sizeof(access))?1:-1;
  logf("Native passive action predicate signature=%d",verified==1);
 }
 if(verified!=1||!actor||chain(playerSlot,{0,4,0})!=actor||(captured&&actor!=lastPlayer))return -1;
 uintptr_t movement=0,action=0,owner=0;uint8_t allowed=255;
 if(!get(actor+0x28,movement)||!movement||(captured&&movement!=lastMap)||
    !get(movement+0x2c,action)||!action||!get(action+0x10,owner)||owner!=movement||
    !get(action+0x3c,allowed)||allowed>1)return -1;
 return allowed;
}

static bool nativeReactionTick(double& acc,bool focus){
 if(!nativeReaction){
  if(!equipmentCapable())return false;
  const auto current=equipmentRead();
  if(!current.valid||(!current.right.melee&&!current.left.shield))return false;
 }
 int allowed=nativeActiveAllowed();
 if(allowed<0){logf("Native reaction predicate invalid; native handback");releaseControl();acc=0;return true;}
 // Active-action ownership is also disabled by the native attack we inject.
 // It is not, on its own, evidence that an enemy hit or broke our guard.
 DWORD observed=GetTickCount();int observedHP=read<int>(player+0x2d4);
 if(!nativeReaction){
  if(observedHP>0&&observedHP<lastHP){
   if(equipmentConsumeNativeBlock(player,lastHP,observedHP)){
    lastHP=observedHP;nativeReactionIncomingUntil=0;
   }else nativeReactionIncomingUntil=observed+200;
  }
  if(equipmentConsumeNativeGuardBreak(player))nativeReactionIncomingUntil=observed+200;
  if(!SCCombat::pending(nativeReactionIncomingUntil,observed))return false;
 }
 if(!nativeReaction&&allowed==1)return false;
 Vec host{};float nativeYaw=0;int hp=0,stamina=0;
 if(!characterOwned()||nativeActionOwned()||character<0||!posData||!cam||
    !get(posData+0x10,host)||!isfinite(host.x)||!isfinite(host.y)||!isfinite(host.z)||
    !get(posData+4,nativeYaw)||!isfinite(nativeYaw)||
    !get(player+0x2d4,hp)||hp<=0||!get(player+0x2e4,stamina)||stamina< -50||
    (nativeReaction&&(currentMap()!=nativeReactionMap||dot(host-nativeReactionPrevious,host-nativeReactionPrevious)>64))){
  logf("Native reaction identity/position invalid; native handback");releaseControl();acc=0;return true;
 }
 DWORD now=GetTickCount();
 if(!nativeReaction){
  nativeReaction=true;nativeReactionStarted=now;nativeReactionMap=currentMap();
  nativeReactionStartHP=hp;nativeReactionStartStamina=stamina;nativeReactionReturnGate.clear();
  worldRelease();combatReset();actionReset();characterClock.clear();characterInput.clear();pendingNativeHurt=0;testFrames=testButtons=0;
  // Emit the reaction pose immediately, then keep ordinary fixed-rate posing.
  acc=1./SC_FIXED_HZ;
  logf("Native reaction begin hp=%d->%d stamina=%d active_allowed=%d",lastHP,hp,stamina,allowed);
 }
 nativeReactionPrevious=host;
 if(hp!=lastHP)logf("Native reaction health %d->%d stamina=%d",lastHP,hp,stamina);
 lastHP=hp;
 actionCancelPulse();InterlockedExchange(&equipmentGuardInput,0);
 InterlockedExchange(&actionRoute,LONG(ActionRoute::Locked));actionFlags(true);
 if(hideNative){put(player+0x258,0.f);put(player+0x1fc,read<uint32_t>(player+0x1fc)&~0x800000u);}
 Vec local=(host-origin)*100;
 if(fabsf(local.x)>4000||fabsf(local.y)>8000||fabsf(local.z)>4000){origin=host;rebuildSurfaces();local={0,0,0};}
 SC_Pose pose={};pose.kind=SC_POSE_STAGGER;pose.phase=-1;pose.yaw=nativeYaw-3.14159265f;
 pose.position[0]=local.x;pose.position[1]=local.y;pose.position[2]=local.z;
 for(int step=0;acc>=1./SC_FIXED_HZ&&step<3;step++,acc-=1./SC_FIXED_HZ){
  if(!guestPose(character,&pose,&state,&geo)){logf("Native reaction pose rejected; native handback");releaseControl();acc=0;return true;}
  pumpAudio();
 }
 // The installed passive ESD state0 enables active actions on entry and
 // disables them on exit. Native recovery, not a guessed duration, ends this.
 if(nativeReactionReturnGate.update(allowed==1&&focus,now)){
  guest->resume(character,local.x,local.y,local.z,pose.yaw);
  state.position[0]=local.x;state.position[1]=local.y;state.position[2]=local.z;
  state.velocity[0]=state.velocity[1]=state.velocity[2]=0;state.forwardSpeed=0;
  state.faceAngle=pose.yaw;state.flags=0;state.attackId=state.interruptedAttackId=-1;state.landingRecoveryTicks=0;
  pendingNativeHurt=0;lastHP=hp;nativeReaction=false;nativeReactionIncomingUntil=0;nativeReactionReturnGate.clear();
  worldActivate();characterClock.clear();characterInput.clear();actionFlags(false);
  InterlockedExchange(&actionRoute,LONG(ActionRoute::Character));acc=0;
  logf("Native reaction end duration_ms=%lu hp=%d->%d stamina=%d->%d",now-nativeReactionStarted,nativeReactionStartHP,hp,nativeReactionStartStamina,stamina);
 }
 return true;
}
