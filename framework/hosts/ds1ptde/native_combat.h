#pragma once
#include "param_templates.h"
// Exact PTDE executable only; private rows never replace native ladder rows.
static unsigned char characterAttacks[SC_MAX_ATTACKS][128],characterBehaviors[SC_MAX_ATTACKS][32];
static bool characterDamageReady=false;
extern "C" { void* characterOriginalAttackLookup=nullptr; void* characterOriginalBehaviorLookup=nullptr; }
extern "C" { void* characterOriginalStamina=nullptr; }
extern "C" __attribute__((used,noinline)) int characterTraceStamina(uintptr_t actor,int value,uintptr_t caller){
 return combatNativeStamina(actor,value,caller);
}
__attribute__((naked)) static void characterStaminaHook(){__asm__ volatile(
 "pushfl\n pushal\n movl 36(%esp),%eax\n movl 40(%esp),%ecx\n pushl %eax\n pushl %ecx\n pushl %edx\n call _characterTraceStamina\n addl $12,%esp\n movl %eax,40(%esp)\n popal\n popfl\n jmp *_characterOriginalStamina\n");}
extern "C" __attribute__((used,noinline)) uintptr_t characterLookup(int id,int kind,uintptr_t out,int behavior){
 if(id<990050||id>=990050+int(profile->attackCount))return 0;
 logf("Character lookup id=%d kind=%d behavior=%d",id,kind,behavior);
 if(kind!=1)return 0;
 int i=id-990050;uintptr_t row=(uintptr_t)(behavior?characterBehaviors[i]:characterAttacks[i]);
 put<int>(out,id);put<int>(out+4,kind);put<uintptr_t>(out+8,row);return row;
}
// Both native lookup ABIs: EDI=id, ECX=kind, stack[1]=out, callee pops 4.
#define SC_LOOKUP_STUB(name,original,behavior) \
__attribute__((naked)) static void name(){__asm__ volatile( \
 "pushfl\n pushal\n movl 40(%esp),%eax\n pushl $" #behavior "\n pushl %eax\n pushl %ecx\n pushl %edi\n call _characterLookup\n addl $16,%esp\n testl %eax,%eax\n jz 1f\n movl %eax,28(%esp)\n popal\n popfl\n ret $4\n 1: popal\n popfl\n jmp *_" #original "\n");}
SC_LOOKUP_STUB(characterAttackLookup,characterOriginalAttackLookup,0)
SC_LOOKUP_STUB(characterBehaviorLookup,characterOriginalBehaviorLookup,1)
#undef SC_LOOKUP_STUB
using CharacterBuildPacket=bool(__cdecl*)(uintptr_t,int,uintptr_t,uintptr_t,int,int,float);
static CharacterBuildPacket characterOriginalPacket=nullptr;
static thread_local int characterPacketMove=-1;
static thread_local float characterPacketPower=0,characterPacketPoise=0;
static thread_local bool characterPacketBuilt=false;
static bool __cdecl characterBuildPacket(uintptr_t out,int id,uintptr_t effects,uintptr_t context,int a,int b,float c){
 bool ok=characterOriginalPacket(out,id,effects,context,a,b,c);
 if(ok&&characterPacketMove>=0&&read<uintptr_t>(context)==player&&read<int>(context+4)==990050+characterPacketMove){
  // D1FE20 constructs pre-defense attack data. EA6CC0 writes the four
  // elemental powers at 0..12; EA6850 writes poise at 0x28. Native target
  // defenses, block checks, damage, death and rewards execute downstream.
  float prior=read<float>(out);put<float>(out,characterPacketPower);
  for(int off:{4,8,12})put<float>(out+off,0.f);
  put<float>(out+0x28,characterPacketPoise);
  // No weapon status/SpEffect inheritance or weapon attack multipliers.
  for(int off=0x6c;off<=0x88;off+=4)put<int>(out+off,-1);
  for(int off:{0x58,0x8c,0x90,0x94,0x98,0x9c,0xa0,0xa4})put<float>(out+off,1.f);
  characterPacketBuilt=true;logf("character packet move=%d power=%.2f native_before=%.2f poise=%.1f",characterPacketMove,characterPacketPower,prior,characterPacketPoise);
 }return ok;
}
static bool characterInstallDamage(){
 if(characterDamageReady)return true;
 const unsigned char lookup[]={0x53,0x8b,0x5c,0x24,0x08,0x33,0xc0};
 const unsigned char packet[]={0x83,0xec,0x34,0x53,0x8b,0x5c,0x24,0x48};
 unsigned char bytes[8];
 for(uintptr_t at:{uintptr_t(0x1004f90),uintptr_t(0xd204e0)}){if(!get(at,bytes)||memcmp(bytes,lookup,sizeof(lookup))){logf("Character damage lookup signature refused");return false;}}
 if(!get(uintptr_t(0xd1fe20),bytes)||memcmp(bytes,packet,sizeof(packet))){logf("Character packet signature refused");return false;}
 for(int i=0;i<int(profile->attackCount);i++){memcpy(characterAttacks[i],characterAttackTemplate,128);memcpy(characterBehaviors[i],characterBehaviorTemplate,32);int id=990050+i;memcpy(characterBehaviors[i]+12,&id,4);}
 const unsigned char stamina[]={0x51,0x8b,0x4c,0x24,0x08};if(!get(uintptr_t(0xe67960),bytes)||memcmp(bytes,stamina,sizeof(stamina)))return false;
 void* addresses[]={(void*)0x1004f90,(void*)0xd204e0,(void*)0xd1fe20,(void*)0xe67960};
 void* hooks[]={(void*)characterBehaviorLookup,(void*)characterAttackLookup,(void*)characterBuildPacket,(void*)characterStaminaHook};
 void** originals[]={&characterOriginalBehaviorLookup,&characterOriginalAttackLookup,(void**)&characterOriginalPacket,&characterOriginalStamina};
 for(int i=0;i<4;i++)if(MH_CreateHook(addresses[i],hooks[i],originals[i])!=MH_OK){for(int j=0;j<i;j++)MH_RemoveHook(addresses[j]);return false;}
 for(int i=0;i<4;i++)if(MH_EnableHook(addresses[i])!=MH_OK){for(auto p:addresses){MH_DisableHook(p);MH_RemoveHook(p);}return false;}
 characterDamageReady=true;logf("Character-owned native attack templates ready");return true;
}
static int characterNativeHit(int move,float power,float poise,int target,Vec position){
 if(!characterDamageReady||move<0||move>=int(profile->attackCount))return -1;
 uint16_t base=(uint16_t)std::clamp(power,0.f,2000.f),stagger=(uint16_t)poise;
 memcpy(characterAttacks[move]+0x50,&base,2);memcpy(characterAttacks[move]+0x5e,&stagger,2);
 characterPacketMove=move;characterPacketPower=power;characterPacketPoise=poise;characterPacketBuilt=false;
 int staminaBefore=read<int>(player+0x2e4);
 int result=nativeDamage(990050+move,10000,target,position.x,position.y,position.z);
 logf("Character native call stamina before=%d after=%d",staminaBefore,read<int>(player+0x2e4));
 characterPacketMove=-1;if(!characterPacketBuilt){logf("Character damage packet was not constructed; disabling outgoing damage");characterDamageReady=false;}
 return result;
}
