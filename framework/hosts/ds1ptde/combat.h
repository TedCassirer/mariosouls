#pragma once
#include "target_geometry.h"
static SCCombat::Settings characterBalance;
static SCCombat::Window characterWindow;
static DWORD characterRecoveryUntil=0;
static int characterStaminaCeiling=0,characterLandingRecovery=0;
static float characterArmorPenalty=0;
static bool characterFalling=false;
static float characterPeakY=0;
static int characterRecoveryDelay=500;
static int combatNativeStamina(uintptr_t actor,int value,uintptr_t caller,uintptr_t packet){
 if(!characterOwned()||nativeActionOwned()||actor!=player||actor!=lastPlayer)return value;
 int current=read<int>(actor+0x2e4);
 equipmentObserveNativeBlock(actor,current,value,caller,packet);
 if(value<current&&InterlockedCompareExchange(&equipmentGuardInput,0,0))logf("Equipment guard stamina before=%d after=%d caller=%08x native_guard=%d",current,value,unsigned(caller),equipmentNativeGuard());
 // The native character-state command1001 (FE024A ->46D550 ->E67960)
 // debits20 after the character's injected hit. Character owns attack/movement costs
 // while active. Incoming hit/block drain and effect routines still pass.
 if(caller==0xfe024f&&value<current){logf("Character skipped duplicate native action stamina %d",current-value);return current;}
 // Delay ordinary regeneration before it reaches the shared native pool.
 if(caller==0xe80c13&&value>current&&SCCombat::pending(characterRecoveryUntil,GetTickCount()))return std::min(current,characterStaminaCeiling);
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
static std::unordered_map<int,DWORD> characterLastStagger;
static std::unordered_map<int,SCImpact::Cooldown> characterLastPush;
static char combatConfigSection[96]={};
static float combatFloat(const char* key,float fallback,float lo,float hi){char d[40],v[40];snprintf(d,sizeof(d),"%.4f",fallback);GetPrivateProfileStringA(combatConfigSection,key,d,v,sizeof(v),ini);float f=strtof(v,nullptr);return isfinite(f)?std::clamp(f,lo,hi):fallback;}
static void combatSettings(){
 feedbackSettings();
 snprintf(combatConfigSection,sizeof(combatConfigSection),"Combat.%s",profile->id);
 characterBalance.base=combatFloat("BasePower",profile->basePower,1,1000);characterBalance.upgradeStep=combatFloat("UpgradePower",profile->upgradePower,0,50);characterBalance.dexterityWeight=profile->dexterityWeight;
 characterBalance.fixedPower=combatFloat("FixedPower",0,0,1200);characterBalance.upgrade=std::clamp<int>(GetPrivateProfileIntA(combatConfigSection,"UpgradeLevel",0,ini),0,15);
 characterBalance.scaling=GetPrivateProfileIntA(combatConfigSection,"StatScaling",1,ini)!=0;
 characterArmorPenalty=combatFloat("EquipmentCostPerWeight",0,0,.02f);

 for(unsigned i=0;i<profile->attackCount;i++){char key[80];snprintf(key,sizeof(key),"%s_multiplier",attackName(i));characterBalance.multiplier[i]=combatFloat(key,profile->attacks[i].multiplier,0,3);snprintf(key,sizeof(key),"%s_cost",attackName(i));characterBalance.poise[i]=profile->attacks[i].poise;characterBalance.cost[i]=std::clamp<int>(GetPrivateProfileIntA(combatConfigSection,key,profile->attacks[i].staminaCost,ini),0,100);}
}
static void combatReset(){equipmentClearNativeBlock();equipmentReset();feedbackReset();characterWindow.reset();characterRecoveryUntil=0;characterStaminaCeiling=0;characterLandingRecovery=0;characterLastStagger.clear();characterLastPush.clear();characterFalling=false;}
static bool combatTestDrop(int height){
 if(!harness.armed||!harnessIdentity()||!characterOwned()||nativeActionOwned()||height<1||height>25||state.position[1]>state.floorHeight+5)return false;
 state.position[1]+=height*100;guest->setPosition(character,state.position[0],state.position[1],state.position[2]);guest->command(character,SC_COMMAND_FALL);state.flags=0;
 Vec p=origin+Vec{state.position[0],state.position[1],state.position[2]}*.01f;
 put(posData+0x10,p);put(lastMap+0xd0,p);put<uint8_t>(lastMap+0xc8,1);worldActivate();characterClock.clear();characterInput.clear();characterFalling=true;characterPeakY=p.y;
 harnessEvent("test_drop");return true;
}
static bool combatEnvironment(){
 float y=origin.y+state.position[1]*.01f;
 bool grounded=bool(state.flags&SC_GROUNDED);
 if(!grounded){if(!characterFalling){characterPeakY=y;characterFalling=true;}characterPeakY=std::max(characterPeakY,y);}
 if(!characterFalling)return true;
 float drop=characterPeakY-y;int maxHP=read<int>(player+0x2d8),hp=read<int>(player+0x2d4);
 float safe=combatFloat("SafeFallMeters",8,6,20),lethal=combatFloat("LethalFallMeters",20,safe+2,50);
 if(grounded||(!characterBody.nonlethalFalls&&drop>=lethal)){
  int base=SCCombat::fallDamage(drop,maxHP,safe,lethal);int damage=SCBody::landingDamage(base,hp,maxHP,characterBody);characterFalling=false;
  // Only while Character owns gravity. Native interactions/falling use native logic.
  if(damage>0&&hp>0&&maxHP>0&&maxHP<100000&&(read<uint32_t>(player+0x1fc)&0x4000)&&player==lastPlayer){
   int after=std::max(0,hp-damage);put<int>(player+0x2d4,after);
   char fields[220];snprintf(fields,sizeof(fields),",\"drop_meters\":%.3f,\"before\":%d,\"after\":%d,\"damage\":%d",drop,hp,after,hp-after);harnessEvent("environmental_fall_damage",fields);logf("Character fall%s",fields);
   if(after==0){if(harness.armed)harnessObserve(true,pollController(GetTickCount()).connected);releaseControl(true);return false;}
  }
 }return true;
}
static void combatInput(SC_Input& in){
 int stamina=read<int>(player+0x2e4);
 if(SCCombat::pending(characterRecoveryUntil,GetTickCount())){characterStaminaCeiling=std::min(characterStaminaCeiling,stamina);if(stamina>characterStaminaCeiling)put<int>(player+0x2e4,characterStaminaCeiling);}
 if(characterLandingRecovery>0){in.jump=in.attack=in.crouch=0;in.moveX=in.moveY=0;--characterLandingRecovery;}
}
static void combatPay(){
 int move=characterWindow.move;if(move<0)return;
 uintptr_t stats=chain(0x1378700,{0,8});int str=read<int>(stats+0x50),dex=read<int>(stats+0x58);
 float weight=combatEquipmentWeight(stats);int stamina=read<int>(player+0x2e4),cost=int(ceilf(characterBalance.cost[move]*(1+std::min(.5f,weight*characterArmorPenalty))));float power=SCCombat::power(characterBalance,str,dex)*characterBalance.multiplier[move];
 characterRecoveryDelay=500+(characterArmorPenalty>0?int(std::min(500.f,weight*8)):0);
 bool paid=characterWindow.pay(stamina,cost,power);
 if(paid){characterStaminaCeiling=stamina-cost;put<int>(player+0x2e4,characterStaminaCeiling);characterRecoveryUntil=GetTickCount()+characterRecoveryDelay;}
 char fields[350];snprintf(fields,sizeof(fields),",\"move\":\"%s\",\"serial\":%d,\"paid\":%s,\"stamina_before\":%d,\"cost\":%d,\"power\":%.3f,\"strength\":%d,\"dexterity\":%d,\"upgrade\":%d",attackName(move),characterWindow.serial,paid?"true":"false",stamina,characterWindow.charged,power,str,dex,characterBalance.upgrade);
 size_t len=strlen(fields);snprintf(fields+len,sizeof(fields)-len,",\"equipment_weight\":%.2f,\"required_cost\":%d,\"recovery_ms\":%d",weight,cost,characterRecoveryDelay);
 harnessEvent("character_attack",fields);logf("Character attack%s",fields);
}
static void combat(){
 if(!combatEnabled||!nativeDamage||!characterDamageReady||(harness.armed&&harness.suppressOutgoing))return;
 bool grounded=bool(state.flags&SC_GROUNDED);
 int interrupted=state.interruptedAttackId;
 if(interrupted!=SCCombat::None&&interrupted!=characterWindow.move&&interrupted!=state.attackId){
  if(characterWindow.observe(interrupted,false,profile->automaticAttackId))combatPay();
 }
 bool start=characterWindow.observe(state.attackId,grounded,profile->automaticAttackId);if(start)combatPay();
 if(state.landingRecoveryTicks)characterLandingRecovery=int(state.landingRecoveryTicks);
 if(characterWindow.move!=SCCombat::None&&characterWindow.move!=profile->automaticAttackId&&!characterWindow.exhausted&&characterWindow.age<24){characterRecoveryUntil=GetTickCount()+characterRecoveryDelay;characterStaminaCeiling=std::min(characterStaminaCeiling,read<int>(player+0x2e4));}
 uintptr_t root=read<uint32_t>(playerSlot),begin=read<uint32_t>(root+4),end=read<uint32_t>(root+8);if(!begin||end<begin||end-begin>16384)return;
 Vec p=origin+Vec{state.position[0],state.position[1],state.position[2]}*.01f;
 const auto*attack=attackDefinition(characterWindow.move);const auto*automatic=attackDefinition(profile->automaticAttackId);
 for(uintptr_t at=begin;at<end;at+=4){
  uintptr_t npc=read<uint32_t>(at);if(!npc||npc==player)continue;
  int team=read<int>(npc+0x74),hp=read<int>(npc+0x2d4),id=read<int>(npc+0x208);if((team!=6&&team!=7&&team!=9)||hp<=0||hp>100000||id<=0)continue;
  uintptr_t np=chain(npc+0x28,{0,0x1c});if(!np)continue;Vec center=read<Vec>(np+0x10);
  bool downward=characterWindow.move==SCCombat::None||characterWindow.move==profile->automaticAttackId;
  SCContact::Capsule capsule;bool shaped=targetCapsule(npc,center,capsule);
  Vec pos=shaped?targetContact(capsule,p,downward):center+Vec{0,downward?1.f:.7f,0},delta=pos-p;
  float range=attack?attack->rangeCm*.01f:automatic?automatic->rangeCm*.01f:0;
  if(!SCContact::inReach({p.x,p.y,p.z},{pos.x,pos.y,pos.z},range,(attack?attack->minHeightCm:-200)*.01f,(attack?attack->maxHeightCm:140)*.01f))continue;
  if(!lineClear(p+Vec{0,.7f,0},pos)||!worldLineClear(p+Vec{0,.7f,0},pos,npc))continue;
  Vec local=(pos-origin)*100;uint32_t contact=guest->probeContact(character,local.x,local.y,local.z);
  // Invulnerable/knockback actions are never offensive stomp states. Require
  // feet above the enemy and tight horizontal overlap, not the old broad sphere.
  if(characterWindow.move==SCCombat::None&&automatic&&(contact&SC_CONTACT_AUTOMATIC)&&(state.flags&SC_AUTO_ATTACK_ELIGIBLE)&&automaticContact(delta,*automatic)){if(characterWindow.beginAutomatic(profile->automaticAttackId))combatPay();}
  int move=characterWindow.move;
  if(!characterWindow.active(state,*profile)||!characterWindow.canHit(id,*profile))continue;
  if(move==profile->automaticAttackId){if(!automatic||!(contact&SC_CONTACT_AUTOMATIC)||!automaticContact(delta,*automatic))continue;}
  else if(!(contact&SC_CONTACT_ATTACK))continue;
  if(read<uint32_t>(read<uint32_t>(0x137d644)+0x3c)!=player)continue;
  bool first=characterWindow.hit.empty();characterWindow.hit.insert(id);
  debugContact(pos,false);
  char fields[240];snprintf(fields,sizeof(fields),",\"move\":\"%s\",\"serial\":%d,\"target_id\":%d,\"power\":%.3f",attackName(move),characterWindow.serial,id,characterWindow.attackPower);harnessEvent("character_contact",fields);
  DWORD now=GetTickCount();float poise=characterBalance.poise[move],push=-1;
  bool pushAttack=characterImpactSupported&&characterImpact.knockbackCm[move]>0;
  if(pushAttack){
   auto&cooldown=characterLastPush[id];bool ready=cooldown.ready(npc,now,characterImpact.cooldownMs);
   float current=NAN,maximum=NAN;get(npc+0x1c0,current);get(npc+0x1c4,maximum);
   if(!ready)poise=0;
   push=shaped&&ready?SCImpact::distance(characterImpact.knockbackCm[move],current,maximum,poise,characterImpact.resistantPoise,capsule.radius,capsule.top+capsule.radius):0;
   if(push>0)cooldown.commit(npc,now);
   logf("Character push target=%d move=%s poise=%.2f/%.2f attack_poise=%.2f requested_m=%.3f ready=%d radius=%.3f height=%.3f",id,attackName(move),current,maximum,poise,push,ready,shaped?capsule.radius:0,shaped?capsule.top+capsule.radius:0);
  }else{auto last=characterLastStagger.find(id);if(last!=characterLastStagger.end()&&DWORD(now-last->second)<1000)poise=0;else characterLastStagger[id]=now;}
  harnessHitAttempt(id,hp,990050+move,0,"hit_attempt");int result=characterNativeHit(move,characterWindow.attackPower,poise,id,pos,push);harnessHitAttempt(id,hp,990050+move,result,"native_call_return");
  if(!characterPacketBuilt){if(harness.armed)harnessStop("native_damage_pipeline_unavailable");return;}
  feedbackTrack(npc,id,hp,characterWindow.serial,move,pos+unit(p+Vec{0,.7f,0}-pos)*.05f);
  // Commit the guest contact reaction once, after eligibility/stamina checks. The
  // read-only probe never bounces an exhausted Character or resets an attack window.
  if(first&&characterPacketBuilt)guest->commitContact(character,local.x,local.y,local.z,160);
  logf("Character hit target=%d move=%s serial=%d hp_before=%d native_result=%d body=%d contact=%.3f/%.3f/%.3f",id,attackName(move),characterWindow.serial,hp,result,shaped,pos.x,pos.y,pos.z);
 }
}
