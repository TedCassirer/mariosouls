#pragma once
static MarioCombat::Settings marioBalance;
static MarioCombat::Window marioWindow;
static DWORD marioRecoveryUntil=0;
static int marioStaminaCeiling=0,marioLandingRecovery=0;
static float marioArmorPenalty=0;
static bool marioFalling=false;
static float marioPeakY=0;
static int marioRecoveryDelay=500;
static int combatNativeStamina(uintptr_t actor,int value,uintptr_t caller){
 if(!enabled||interacting||actor!=player||actor!=lastPlayer)return value;
 int current=read<int>(actor+0x2e4);
 // The native character-state command1001 (FE024A ->46D550 ->E67960)
 // debits20 after Mario's injected hit. Mario owns attack/movement costs
 // while active. Incoming hit/block drain and effect routines still pass.
 if(caller==0xfe024f&&value<current){logf("Mario skipped duplicate native action stamina %d",current-value);return current;}
 // Delay ordinary regeneration before it reaches the shared native pool.
 if(caller==0xe80c13&&value>current&&MarioCombat::pending(marioRecoveryUntil,GetTickCount()))return std::min(current,marioStaminaCeiling);
 return value;
}
static float combatEquipmentWeight(uintptr_t stats){
 uintptr_t repo=read<uint32_t>(0x13785b4);float total=0;
 for(int slot:{0x24c,0x250,0x254,0x258,0x26c,0x270,0x274,0x278}){
  int id=read<int>(stats+slot);if(id<0)continue;id-=id%1000;
  bool armor=slot>=0x26c;uintptr_t param=chain(repo+(armor?36:0)+12,{0,0x20});char type[32]={};
  if(!get(param+12,type)||strcmp(type,armor?"EQUIP_PARAM_PROTECTOR_ST":"EQUIP_PARAM_WEAPON_ST"))return 50.f;
  int count=read<uint16_t>(param+10);if(count<1||count>5000)return 50.f;
  int lo=0,hi=count-1;bool found=false;
  while(lo<=hi){int mid=(lo+hi)/2,key=read<int>(param+0x30+mid*12);if(key<id)lo=mid+1;else if(key>id)hi=mid-1;else{
   uint32_t offset=read<uint32_t>(param+0x34+mid*12);float weight=offset<2000000?read<float>(param+offset+(armor?0x20:0xc)):-1;
   if(!isfinite(weight)||weight<0||weight>50)return 50.f;total+=weight;found=true;break;
  }}if(!found)return 50.f;
 }return total;
}
static std::unordered_map<int,DWORD> marioLastStagger;
static float combatFloat(const char* key,float fallback,float lo,float hi){char d[40],v[40];snprintf(d,sizeof(d),"%.4f",fallback);GetPrivateProfileStringA("Combat",key,d,v,sizeof(v),ini);float f=strtof(v,nullptr);return isfinite(f)?std::clamp(f,lo,hi):fallback;}
static void combatSettings(){
 marioBalance.base=combatFloat("BasePower",100,1,1000);marioBalance.upgradeStep=combatFloat("UpgradePower",10,0,50);
 marioBalance.fixedPower=combatFloat("FixedPower",0,0,1200);marioBalance.upgrade=std::clamp<int>(GetPrivateProfileIntA("Combat","UpgradeLevel",0,ini),0,15);
 marioBalance.scaling=GetPrivateProfileIntA("Combat","StatScaling",1,ini)!=0;
 marioArmorPenalty=combatFloat("EquipmentCostPerWeight",0,0,.02f);
 MarioCombat::Settings defaults;
 for(int i=0;i<8;i++){char key[80];snprintf(key,sizeof(key),"%s_multiplier",MarioCombat::name(i));marioBalance.multiplier[i]=combatFloat(key,defaults.multiplier[i],0,3);snprintf(key,sizeof(key),"%s_cost",MarioCombat::name(i));marioBalance.cost[i]=std::clamp<int>(GetPrivateProfileIntA("Combat",key,defaults.cost[i],ini),0,100);}
}
static void combatReset(){marioWindow.reset();marioRecoveryUntil=0;marioStaminaCeiling=0;marioLandingRecovery=0;marioLastStagger.clear();marioFalling=false;}
static bool combatTestDrop(int height){
 if(!harness.armed||!harnessIdentity()||!enabled||interacting||height<1||height>25||state.position[1]>state.floorHeight+5)return false;
 state.position[1]+=height*100;sm_position(mario,state.position[0],state.position[1],state.position[2]);sm_action(mario,0x0100088c);state.action=0x0100088c;
 Vec p=origin+Vec{state.position[0],state.position[1],state.position[2]}*.01f;
 put(posData+0x10,p);put(lastMap+0xd0,p);put<uint8_t>(lastMap+0xc8,1);marioFalling=true;marioPeakY=p.y;
 harnessEvent("test_drop");return true;
}
static bool combatEnvironment(){
 float y=origin.y+state.position[1]*.01f;
 bool grounded=!(state.action&0x800)&&state.position[1]<=state.floorHeight+5;
 if(!grounded){if(!marioFalling){marioPeakY=y;marioFalling=true;}marioPeakY=std::max(marioPeakY,y);}
 if(!marioFalling)return true;
 float drop=marioPeakY-y;int maxHP=read<int>(player+0x2d8),hp=read<int>(player+0x2d4);
 float safe=combatFloat("SafeFallMeters",8,6,20),lethal=combatFloat("LethalFallMeters",20,safe+2,50);
 if(grounded||drop>=lethal){
  int damage=MarioCombat::fallDamage(drop,maxHP,safe,lethal);marioFalling=false;
  // Only while Mario owns gravity. Native interactions/falling use native logic.
  if(damage>0&&hp>0&&maxHP>0&&maxHP<100000&&(read<uint32_t>(player+0x1fc)&0x4000)&&player==lastPlayer){
   int after=std::max(0,hp-damage);put<int>(player+0x2d4,after);
   char fields[220];snprintf(fields,sizeof(fields),",\"drop_meters\":%.3f,\"before\":%d,\"after\":%d,\"damage\":%d",drop,hp,after,hp-after);harnessEvent("environmental_fall_damage",fields);logf("Mario fall%s",fields);
   if(after==0){if(harness.armed)harnessObserve(true,pollController(GetTickCount()).connected);releaseControl();return false;}
  }
 }return true;
}
static void combatInput(SM64MarioInputs& in){
 int stamina=read<int>(player+0x2e4);
 if(MarioCombat::pending(marioRecoveryUntil,GetTickCount())){marioStaminaCeiling=std::min(marioStaminaCeiling,stamina);if(stamina>marioStaminaCeiling)put<int>(player+0x2e4,marioStaminaCeiling);}
 if(marioLandingRecovery>0){in.buttonA=in.buttonB=in.buttonZ=0;in.stickX=in.stickY=0;--marioLandingRecovery;}
}
static void combatPay(){
 int move=marioWindow.move;if(move<0)return;
 uintptr_t stats=chain(0x1378700,{0,8});int str=read<int>(stats+0x50),dex=read<int>(stats+0x58);
 float weight=combatEquipmentWeight(stats);int stamina=read<int>(player+0x2e4),cost=int(ceilf(marioBalance.cost[move]*(1+std::min(.5f,weight*marioArmorPenalty))));float power=MarioCombat::power(marioBalance,str,dex)*marioBalance.multiplier[move];
 marioRecoveryDelay=500+(marioArmorPenalty>0?int(std::min(500.f,weight*8)):0);
 bool paid=marioWindow.pay(stamina,cost,power);
 if(paid){marioStaminaCeiling=stamina-cost;put<int>(player+0x2e4,marioStaminaCeiling);marioRecoveryUntil=GetTickCount()+marioRecoveryDelay;}
 char fields[350];snprintf(fields,sizeof(fields),",\"move\":\"%s\",\"serial\":%d,\"paid\":%s,\"stamina_before\":%d,\"cost\":%d,\"power\":%.3f,\"strength\":%d,\"dexterity\":%d,\"upgrade\":%d",MarioCombat::name(move),marioWindow.serial,paid?"true":"false",stamina,marioWindow.charged,power,str,dex,marioBalance.upgrade);
 size_t len=strlen(fields);snprintf(fields+len,sizeof(fields)-len,",\"equipment_weight\":%.2f,\"required_cost\":%d,\"recovery_ms\":%d",weight,cost,marioRecoveryDelay);
 harnessEvent("mario_attack",fields);logf("Mario attack%s",fields);
}
static void combat(){
 if(!combatEnabled||!nativeDamage||!marioDamageReady||(harness.armed&&harness.suppressOutgoing))return;
 bool grounded=!(state.action&0x800)&&state.position[1]<=state.floorHeight+5;
 int previous=marioWindow.move;
 // A dive can enter and collide with a wall in one SM64 tick. Charge that
 // committed attack even though the final action is already knockback.
 int interrupted=MarioCombat::identify(state.interruptedAction,0);
 if(interrupted!=MarioCombat::None&&interrupted!=marioWindow.move&&interrupted!=MarioCombat::identify(state.action,state.actionArg)){
  if(marioWindow.observe(state.interruptedAction,0,false))combatPay();
 }
 bool start=marioWindow.observe(state.action,state.actionArg,grounded);
 if(start)combatPay();
 static uint32_t previousAction=0;
 if(state.action==0x0080023c&&previousAction!=state.action)marioLandingRecovery=12;
 previousAction=state.action;
 if(marioWindow.move!=MarioCombat::None&&marioWindow.move!=MarioCombat::Stomp&&!marioWindow.exhausted&&marioWindow.age<24){marioRecoveryUntil=GetTickCount()+marioRecoveryDelay;marioStaminaCeiling=std::min(marioStaminaCeiling,read<int>(player+0x2e4));}
 uintptr_t root=read<uint32_t>(playerSlot),begin=read<uint32_t>(root+4),end=read<uint32_t>(root+8);if(!begin||end<begin||end-begin>16384)return;
 Vec p=origin+Vec{state.position[0],state.position[1],state.position[2]}*.01f;
 bool pound=marioWindow.move==MarioCombat::Pound;
 for(uintptr_t at=begin;at<end;at+=4){
  uintptr_t npc=read<uint32_t>(at);if(!npc||npc==player)continue;
  int team=read<int>(npc+0x74),hp=read<int>(npc+0x2d4),id=read<int>(npc+0x208);if((team!=6&&team!=7&&team!=9)||hp<=0||hp>100000||id<=0)continue;
  uintptr_t np=chain(npc+0x28,{0,0x1c});if(!np)continue;Vec pos=read<Vec>(np+0x10),delta=pos-p;
  float range=pound?1.65f:1.25f;if(!isfinite(dot(delta,delta))||delta.x*delta.x+delta.z*delta.z>range*range||delta.y>1.4f||delta.y< -2.f)continue;
  if(!lineClear(p+Vec{0,.7f,0},pos+Vec{0,.7f,0})||!worldLineClear(p+Vec{0,.7f,0},pos+Vec{0,.7f,0},npc))continue;
  Vec local=(pos-origin)*100;uint32_t contact=sm_probe(mario,local.x,local.y,local.z);
  // Invulnerable/knockback actions are never offensive stomp states. Require
  // feet above the enemy and tight horizontal overlap, not the old broad sphere.
  if(marioWindow.move==MarioCombat::None&&(contact&0x40)&&!(state.action&0x20000)&&delta.y<-.35f&&delta.y>-1.7f&&delta.x*delta.x+delta.z*delta.z<.55f*.55f){if(marioWindow.beginStomp())combatPay();}
  int move=marioWindow.move;
  if(!marioWindow.active(state.action,state.flags,state.animFrame,state.velocity[1],state.actionTimer)||!marioWindow.canHit(id))continue;
  if(move==MarioCombat::Stomp){if(!(contact&0x40)||delta.y>=-.35f||delta.y<=-1.7f||delta.x*delta.x+delta.z*delta.z>=.55f*.55f)continue;}
  else if(!(contact&0x3f))continue;
  if(read<uint32_t>(read<uint32_t>(0x137d644)+0x3c)!=player)continue;
  bool first=marioWindow.hit.empty();marioWindow.hit.insert(id);
  char fields[240];snprintf(fields,sizeof(fields),",\"move\":\"%s\",\"serial\":%d,\"target_id\":%d,\"power\":%.3f",MarioCombat::name(move),marioWindow.serial,id,marioWindow.attackPower);harnessEvent("mario_contact",fields);
  DWORD now=GetTickCount();float poise=marioBalance.poise[move];auto last=marioLastStagger.find(id);if(last!=marioLastStagger.end()&&DWORD(now-last->second)<1000)poise=0;else marioLastStagger[id]=now;
  harnessHitAttempt(id,hp,990050+move,0,"hit_attempt");int result=marioNativeHit(move,marioWindow.attackPower,poise,id,pos);harnessHitAttempt(id,hp,990050+move,result,"native_call_return");
  if(!marioPacketBuilt){if(harness.armed)harnessStop("native_damage_pipeline_unavailable");return;}
  // Commit SM64 contact reaction once, after eligibility/stamina checks. The
  // read-only probe never bounces an exhausted Mario or resets an attack window.
  if(first&&marioPacketBuilt)sm_attack(mario,local.x,local.y,local.z,160);
  logf("Mario hit target=%d move=%s serial=%d hp_before=%d native_result=%d",id,MarioCombat::name(move),marioWindow.serial,hp,result);
 }
}
