#pragma once
#include "equipment_family.h"
#include <vector>
#include <unordered_map>
// PTDE 1.0.2.0 equipment reads and native damage dispatch. No inventory edits.
// All addresses are host-private. Adapters receive only SDK equipment snapshots.
struct NativeEquipmentAttack {
 int behavior=-1,attack=-1,stamina=0;
 bool valid=false;
};
struct NativeEquipmentItem {
 bool valid=false,empty=false,melee=false,shield=false,pyromancy=false;
 uint32_t kind=SC_EQUIPMENT_NONE;
 int id=-1,rowId=-1,model=-1,variation=-1,category=-1,animation=-1,guardAnimation=-1;
 int upgrade=0,reinforce=-1,requiredStrength=0,requiredDexterity=0;
 float weight=0,guard[4]={},stability=0;
 // Guard percentages include the reinforcement row; native effects are still
 // resolved by the native guard routine, not by these display/eligibility fields.
 NativeEquipmentAttack attacks[2];
};
struct NativeEquipment {
 bool valid=false,oneHanded=false;
 int stance=-1,rightSlot=-1,leftSlot=-1;
 NativeEquipmentItem right,left;
};

static uintptr_t equipmentParamRow(int slot,const char* expected,int id){
 if(slot<0||slot>=36||id<0)return 0;
 uintptr_t repo=read<uint32_t>(0x13785b4);
 if(!repo||read<int>(repo+slot*36+8)<=0)return 0;
 uintptr_t param=chain(repo+slot*36+12,{0,0x20});
 char type[33]={};unsigned char header[48]={};
 if(!param||!get(param,header))return 0;
 memcpy(type,header+12,32);
 if(strcmp(type,expected))return 0;
 int count=read<uint16_t>(param+10);
 if(count<1||count>20000)return 0;
 // The game's behavior/attack directories contain appended unsorted groups.
 // Cache only directory indices, then re-read and verify the entry every use.
 struct Directory {uintptr_t address=0;int count=0;std::unordered_map<int,int> indices;};
 static Directory directories[36];auto& cache=directories[slot];
 if(cache.address!=param||cache.count!=count){cache={};cache.address=param;cache.count=count;}
 auto found=cache.indices.find(id);int index=found==cache.indices.end()?-1:found->second;
 int entry[3]={};
 if(index<0||!get(param+0x30+index*12,entry)||entry[0]!=id){
  std::vector<int> entries(count*3);SIZE_T got=0;
  if(!ReadProcessMemory(GetCurrentProcess(),(void*)(param+0x30),entries.data(),entries.size()*sizeof(int),&got)||got!=entries.size()*sizeof(int))return 0;
  cache.indices.clear();
  for(int i=0;i<count;i++)if(!cache.indices.emplace(entries[i*3],i).second){cache.indices.clear();return 0;}
  found=cache.indices.find(id);if(found==cache.indices.end())return 0;
  index=found->second;memcpy(entry,entries.data()+index*3,sizeof(entry));
 }
 uint32_t offset=uint32_t(entry[1]);
 if(offset<0x30+count*12||offset>16*1024*1024)return 0;
 return param+offset;
}
static NativeEquipmentAttack equipmentAttack(int variation,int judge){
 NativeEquipmentAttack a;
 if(variation<0||variation>9999||(judge!=0&&judge!=10))return a;
 int behavior=100000000+variation*1000+judge;
 // Native lookup 1004F90: kind1 +12 = PlayerBehaviors slot13.
 uintptr_t row=equipmentParamRow(13,"BEHAVIOR_PARAM_ST",behavior);
 unsigned char bytes[32]={};if(!row||!get(row,bytes))return a;
 int actualVariation=-1,actualJudge=-1,attack=-1,stamina=-1;
 memcpy(&actualVariation,bytes,4);memcpy(&actualJudge,bytes+4,4);
 memcpy(&attack,bytes+12,4);memcpy(&stamina,bytes+20,4);
 if(actualVariation!=variation||actualJudge!=judge||bytes[9]!=0||stamina<=0||stamina>150)return a;
 // D204E0: kind1 +7 = PlayerAttacks slot8. Reject projectile behaviors.
 if(!equipmentParamRow(8,"ATK_PARAM_ST",attack))return a;
 a.behavior=behavior;a.attack=attack;a.stamina=stamina;a.valid=true;return a;
}
static NativeEquipmentItem equipmentItem(int id){
 NativeEquipmentItem item;item.id=id;
 if(id==900000||id==-1){item.valid=true;item.empty=true;return item;}
 if(id<0||id>10000000)return item;
 // D1A950 divides by100: retain infusion/path digits, discard upgrade level only.
 item.rowId=id-id%100;item.upgrade=id%100;
 if(item.upgrade>15)return item;
 uintptr_t row=equipmentParamRow(0,"EQUIP_PARAM_WEAPON_ST",item.rowId);
 unsigned char bytes[264]={};if(!row||!get(row,bytes))return item;
 memcpy(&item.variation,bytes,4);memcpy(&item.weight,bytes+12,4);
 uint16_t model=0;memcpy(&model,bytes+0xb8,2);item.model=model;
 item.category=bytes[0xe2];item.animation=bytes[0xe3];item.guardAnimation=bytes[0xe4];
 item.requiredStrength=bytes[0xed];item.requiredDexterity=bytes[0xee];
 int16_t reinforce=0,stability=0;memcpy(&reinforce,bytes+0xd6,2);memcpy(&stability,bytes+0xd4,2);
 item.reinforce=reinforce+item.upgrade;
 uintptr_t upgrade=equipmentParamRow(4,"REINFORCE_PARAM_WEAPON_ST",item.reinforce);
 float multipliers[20]={};if(!upgrade||!get(upgrade,multipliers))return item;
 if(!isfinite(item.weight)||item.weight<0||item.weight>100)return item;
 for(int i=0;i<4;i++){
  float base=0;memcpy(&base,bytes+0x30+4*i,4);
  item.guard[i]=base*multipliers[11+i];
  if(!isfinite(item.guard[i])||item.guard[i]<0||item.guard[i]>100)return item;
 }
 item.stability=stability*multipliers[19];
 if(!isfinite(item.stability)||item.stability<0||item.stability>100)return item;
 item.kind=equipmentFamily(item.category,item.animation);item.melee=item.kind!=SC_EQUIPMENT_NONE;
 // Owned weapon params: category8 / animation41 and CanCastPyromancy bit.
 item.pyromancy=item.category==8&&item.animation==41&&(bytes[0x101]&1);
 if(item.pyromancy)item.kind=SC_EQUIPMENT_PYROMANCY;
 // Guard animation1 is a greatshield, outside the first supported family.
 item.shield=item.category==12&&(item.guardAnimation==0||item.guardAnimation==2)&&(bytes[0x100]&0x20);
 if(item.melee){
  item.attacks[0]=equipmentAttack(item.variation,0);item.attacks[1]=equipmentAttack(item.variation,10);
  if(!item.attacks[0].valid||!item.attacks[1].valid)item.melee=false;
 }
 item.valid=true;return item;
}
static NativeEquipment equipmentRead(){
 NativeEquipment equipment;
 if(!player||player!=lastPlayer||read<int>(player+0x2d4)<=0)return equipment;
 uintptr_t stats=chain(0x1378700,{0,8});
 if(!stats||!get(stats+0x230,equipment.stance)||!get(stats+0x234,equipment.leftSlot)||!get(stats+0x238,equipment.rightSlot))return equipment;
 if(equipment.stance<0||equipment.stance>3||equipment.leftSlot<0||equipment.leftSlot>1||equipment.rightSlot<0||equipment.rightSlot>1)return equipment;
 int left=-2,right=-2;
 if(!get(stats+0x24c+equipment.leftSlot*8,left)||!get(stats+0x250+equipment.rightSlot*8,right))return equipment;
 equipment.left=equipmentItem(left);equipment.right=equipmentItem(right);
 equipment.oneHanded=equipment.stance==1;
 equipment.valid=equipment.left.valid&&equipment.right.valid;return equipment;
}
static bool equipmentNativeGuard(){
 // Verified via IsTargetGuard(CF2CC0) -> target accessor vtable+8 -> CB68E0.
 // This is the game's current guard predicate, independent of input buttons.
 static int verified=0;
 if(!verified){
  const unsigned char expected[]={0x8b,0x40,0x28,0x8b,0x40,0x2c,0x83,0xc0,0x78,0xf6,0x40,0x08,0x02};
  unsigned char actual[sizeof(expected)]={};verified=get(uintptr_t(0xcb68eb),actual)&&!memcmp(actual,expected,sizeof(expected))?1:-1;
 }
 if(verified!=1||!player||player!=lastPlayer)return false;
 uintptr_t movement=chain(player+0x28,{0,0x2c});
 uint8_t flags=0;uint32_t disabled=0;
 return movement&&get(movement+0x80,flags)&&get(movement+0x17c,disabled)&&(flags&2)&&!(disabled&8);
}

// Presentation-only evidence of an actually resolved native block. Holding
// guard is insufficient: rear hits and guard breaks must retain native hurt.
struct NativeBlockImpact {
 uintptr_t actor=0;int hpBefore=0,hpAfter=0;DWORD observed=0;bool valid=false;
};
static NativeBlockImpact equipmentBlockImpact;
struct NativeGuardBreakImpact {uintptr_t actor=0;int hp=0,stamina=0;DWORD observed=0;bool valid=false;};
static NativeGuardBreakImpact equipmentGuardBreakImpact;
static SRWLOCK equipmentBlockImpactLock=SRWLOCK_INIT;
static void equipmentClearNativeBlock(){
 AcquireSRWLockExclusive(&equipmentBlockImpactLock);equipmentBlockImpact={};equipmentGuardBreakImpact={};ReleaseSRWLockExclusive(&equipmentBlockImpactLock);
}
static bool equipmentBlockPacketLayout(){
 static int verified=0;
 if(!verified){
  // E7EC54 first checks actual guard/disabled state. The directional result
  // reaches E7F4B0, which sets packet+104. E7F510 sets +154 on guard break.
  const unsigned char block[]={0xc6,0x83,0x04,0x01,0x00,0x00,0x01};
  const unsigned char broken[]={0x8b,0x86,0xe4,0x02,0x00,0x00,0x3b,0x83,0x70,0x01,0x00,0x00,0x7f,0x28,0x8b,0x43,0x1c,0xc6,0x83,0x54,0x01,0x00,0x00,0x01};
  // HP commit precedes the stamina call. EBX still owns this resolved packet
  // at E6C1E6, so its +16C damage describes the immediately completed HP loss.
  const unsigned char hp[]={0x8a,0x8b,0xc5,0x00,0x00,0x00,0x8b,0xbb,0x6c,0x01,0x00,0x00,0x80,0xe1,0x01,0xf7,0xdf,0xe8,0x28,0xe3,0xff,0xff};
  const unsigned char stamina[]={0x8b,0x8e,0xe4,0x02,0x00,0x00,0x2b,0xc8,0x51,0x8b,0xd6,0xe8,0x7a,0xb7,0xff,0xff};
  unsigned char a[sizeof(block)]={},b[sizeof(broken)]={},c[sizeof(hp)]={},d[sizeof(stamina)]={};
  verified=get(uintptr_t(0xe7f4b0),a)&&!memcmp(a,block,sizeof(a))&&get(uintptr_t(0xe7f510),b)&&!memcmp(b,broken,sizeof(b))&&
   get(uintptr_t(0xe6c052),c)&&!memcmp(c,hp,sizeof(c))&&get(uintptr_t(0xe6c1d6),d)&&!memcmp(d,stamina,sizeof(d))?1:-1;
  logf("Equipment resolved block packet signature=%d",verified==1);
 }
 return verified==1;
}
static void equipmentObserveNativeBlock(uintptr_t actor,int currentStamina,int value,uintptr_t caller,uintptr_t packet){
 if(caller!=0xe6c1e6||!characterOwned()||nativeActionOwned()||actor!=player||actor!=lastPlayer)return;
 uint8_t blocked=0,broken=255,breakResult=255;int damage=-1,drain=-1,hp=0,maxHP=0;
 bool packetValid=equipmentBlockPacketLayout()&&currentStamina>value&&packet>0x10000&&
  get(packet+0x104,blocked)&&blocked<=1&&get(packet+0x154,broken)&&broken<=1&&get(packet+0x155,breakResult)&&breakResult<=1&&
  get(packet+0x16c,damage)&&damage>=0&&damage<100000&&get(packet+0x170,drain)&&drain==currentStamina-value&&
  get(actor+0x2d4,hp)&&get(actor+0x2d8,maxHP)&&hp>0&&maxHP>0&&maxHP<100000&&damage<=maxHP-hp;
 bool valid=packetValid&&blocked==1&&broken==0&&breakResult==0&&value>0&&damage>0;
 bool guardBreak=packetValid&&broken==1&&(blocked==1||breakResult==1)&&value<=0;
 DWORD now=GetTickCount();
 AcquireSRWLockExclusive(&equipmentBlockImpactLock);
 if(valid){
  int before=hp+damage;
  // Adjacent blocks may land inside one render interval. Aggregate only an
  // exact HP chain; any intervening damage makes the eventual match fail.
  if(equipmentBlockImpact.valid&&equipmentBlockImpact.actor==actor&&DWORD(now-equipmentBlockImpact.observed)<=100&&equipmentBlockImpact.hpAfter==before)
   equipmentBlockImpact.hpAfter=hp;
  else equipmentBlockImpact={actor,before,hp,now,true};
 }else equipmentBlockImpact={};
 if(guardBreak)equipmentGuardBreakImpact={actor,hp,std::max(-50,value),now,true};
 ReleaseSRWLockExclusive(&equipmentBlockImpactLock);
 logf("Equipment native block packet accepted=%d guard_break=%d hp=%d damage=%d drain=%d remaining=%d blocked=%u broken=%u/%u",valid,guardBreak,hp,damage,drain,value,blocked,broken,breakResult);
}
static bool equipmentConsumeNativeBlock(uintptr_t actor,int hpBefore,int hpAfter){
 AcquireSRWLockExclusive(&equipmentBlockImpactLock);
 auto impact=equipmentBlockImpact;equipmentBlockImpact={};
 ReleaseSRWLockExclusive(&equipmentBlockImpactLock);
 bool matched=impact.valid&&characterOwned()&&!nativeActionOwned()&&actor==player&&actor==lastPlayer&&impact.actor==actor&&
  hpAfter>0&&hpBefore>hpAfter&&impact.hpBefore==hpBefore&&impact.hpAfter==hpAfter&&DWORD(GetTickCount()-impact.observed)<=100;
 if(matched)logf("Equipment native block chip presentation retained hp=%d->%d",hpBefore,hpAfter);
 return matched;
}
static bool equipmentConsumeNativeGuardBreak(uintptr_t actor){
 AcquireSRWLockExclusive(&equipmentBlockImpactLock);
 auto impact=equipmentGuardBreakImpact;equipmentGuardBreakImpact={};
 ReleaseSRWLockExclusive(&equipmentBlockImpactLock);
 int hp=0,stamina=0;
 bool matched=impact.valid&&characterOwned()&&!nativeActionOwned()&&actor==player&&actor==lastPlayer&&impact.actor==actor&&
  DWORD(GetTickCount()-impact.observed)<=100&&get(actor+0x2d4,hp)&&hp==impact.hp&&get(actor+0x2e4,stamina)&&stamina==impact.stamina;
 if(matched)logf("Equipment native guard break confirmed hp=%d stamina=%d",hp,stamina);
 return matched;
}

struct NativeEquipmentPacketTrace {
 bool expected=false,built=false,valid=false;
 int behavior=-1,attack=-1,weapon=-1,observedWeapon=-1,hand=-99;
 float power[4]={},poise=0;
};
static thread_local NativeEquipmentPacketTrace equipmentPacket;
// Invoke from the existing D1FE20 hook after the original has built a packet.
static void equipmentObservePacket(uintptr_t out,int id,uintptr_t context){
 if(!equipmentPacket.expected||id!=equipmentPacket.behavior||read<uintptr_t>(context)!=player||read<int>(context+4)!=equipmentPacket.attack)return;
 equipmentPacket.built=true;equipmentPacket.hand=read<int>(context+0x48);equipmentPacket.observedWeapon=read<int>(context+0x4c);
 bool valid=equipmentPacket.hand==-1&&equipmentPacket.observedWeapon==equipmentPacket.weapon;
 for(int i=0;i<4;i++){
  equipmentPacket.power[i]=read<float>(out+i*4);
  valid=valid&&isfinite(equipmentPacket.power[i])&&equipmentPacket.power[i]>=0&&equipmentPacket.power[i]<=10000;
 }
 equipmentPacket.poise=read<float>(out+0x28);equipmentPacket.valid=valid;
 logf("Equipment packet behavior=%d attack=%d weapon=%d observed=%d hand=%d power=%.2f/%.2f/%.2f/%.2f poise=%.2f valid=%d",id,equipmentPacket.attack,equipmentPacket.weapon,equipmentPacket.observedWeapon,equipmentPacket.hand,equipmentPacket.power[0],equipmentPacket.power[1],equipmentPacket.power[2],equipmentPacket.power[3],equipmentPacket.poise,valid);
}
static int equipmentNativeHit(const NativeEquipmentItem& sword,int combo,int target,Vec position){
 if(!nativeDamage||!sword.valid||!sword.melee||combo<0||combo>1||equipmentPacket.expected)return -1;
 auto current=equipmentRead();
 if(!current.valid||!current.oneHanded||!current.right.melee||current.right.id!=sword.id)return -1;
 const auto& attack=sword.attacks[combo];if(!attack.valid)return -1;
 equipmentPacket={};equipmentPacket.expected=true;equipmentPacket.weapon=sword.id;equipmentPacket.behavior=attack.behavior;equipmentPacket.attack=attack.attack;
 int result=nativeDamage(attack.behavior,10000,target,position.x,position.y,position.z);
 equipmentPacket.expected=false;
 if(!equipmentPacket.built||!equipmentPacket.valid){logf("Equipment packet validation failed: built=%d valid=%d",equipmentPacket.built,equipmentPacket.valid);return -1;}
 return result;
}
