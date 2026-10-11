#pragma once
// Hide the complete native player assembly after the native visibility update.
// Equipment remains equipped and its animation, physics and stats keep updating.
struct CharacterVisibilityState {
 uintptr_t actor=0,body=0,assembly=0;
 uint8_t bodyBit=0,assemblyBit=0;
 bool armed=false,bodyCaptured=false,assemblyCaptured=false;
};
static CharacterVisibilityState characterVisibility;
static SRWLOCK characterVisibilityLock=SRWLOCK_INIT;
using CharacterVisibilityFn=void(__thiscall*)(uintptr_t);
static CharacterVisibilityFn characterOriginalVisibility=nullptr;
static bool characterVisibilityReady=false;
static bool visibilityIdentity(uintptr_t actor){
 return actor&&actor==player&&actor==lastPlayer&&chain(playerSlot,{0,4,0})==actor;
}
static bool visibilityBit(uintptr_t model,unsigned flags,unsigned dirty,bool enabled){
 uint8_t value=0;uint32_t changes=0;
 if(!model||!get(model+flags,value)||!get(model+dirty,changes))return false;
 uint8_t next=uint8_t((value&~1u)|(enabled?1u:0u));
 if(next==value)return true;
 // Exact updates made by E4B345..E4B383 / native setter470760.
 if(!put<uint32_t>(model+dirty,changes|0x200u))return false;
 return put<uint8_t>(model+flags,next);
}
static void visibilityCapture(){
 AcquireSRWLockExclusive(&characterVisibilityLock);
 characterVisibility={};
 if(characterVisibilityReady&&hideNative&&visibilityIdentity(player)){
  auto& v=characterVisibility;v.actor=player;
  v.body=read<uint32_t>(player+0x24);v.assembly=read<uint32_t>(player+0x658);
  uint8_t flags=0;
  v.bodyCaptured=v.body&&get(v.body+0x76,flags);v.bodyBit=flags&1;
  flags=0;v.assemblyCaptured=v.assembly&&get(v.assembly+0x92,flags);v.assemblyBit=flags&1;
  v.armed=true;
 }
 ReleaseSRWLockExclusive(&characterVisibilityLock);
}
static void visibilityRelease(){
 AcquireSRWLockExclusive(&characterVisibilityLock);
 auto v=characterVisibility;characterVisibility={};
 if(v.armed&&visibilityIdentity(v.actor)){
  if(v.bodyCaptured&&read<uint32_t>(v.actor+0x24)==v.body)visibilityBit(v.body,0x76,0x68,v.bodyBit!=0);
  if(v.assemblyCaptured&&read<uint32_t>(v.actor+0x658)==v.assembly)visibilityBit(v.assembly,0x92,0x84,v.assemblyBit!=0);
 }
 ReleaseSRWLockExclusive(&characterVisibilityLock);
}
static void __fastcall characterVisibilityHook(uintptr_t actor,uintptr_t){
 characterOriginalVisibility(actor);
 AcquireSRWLockExclusive(&characterVisibilityLock);
 const auto& v=characterVisibility;
 if(v.armed&&hideNative&&characterPresented()&&actor==v.actor&&visibilityIdentity(actor)){
  // Read current model identities after the native call. Equipment reloads can
  // replace models; never write a saved pointer after it leaves this actor.
  uintptr_t body=read<uint32_t>(actor+0x24),assembly=read<uint32_t>(actor+0x658);
  visibilityBit(body,0x76,0x68,false);visibilityBit(assembly,0x92,0x84,false);
 }
 ReleaseSRWLockExclusive(&characterVisibilityLock);
}
static void installVisibility(){
 if(characterVisibilityReady)return;
 // PlayerIns vtable118A5D4 +284 -> E4B2C0, a no-argument thiscall updater.
 const unsigned char signature[]={0xf6,0x81,0xfe,0x01,0x00,0x00,0x80};
 unsigned char bytes[sizeof(signature)]={};
 if(!get(uintptr_t(0xe4b2c0),bytes)||memcmp(bytes,signature,sizeof(bytes))){logf("Native character visibility signature refused");return;}
 auto status=MH_CreateHook((void*)0xe4b2c0,(void*)characterVisibilityHook,(void**)&characterOriginalVisibility);
 if(status==MH_OK){
  status=MH_EnableHook((void*)0xe4b2c0);
  if(status!=MH_OK){MH_RemoveHook((void*)0xe4b2c0);characterOriginalVisibility=nullptr;}
 }
 characterVisibilityReady=status==MH_OK;
 logf("Native character assembly visibility hook=%d ready=%d",status,characterVisibilityReady);
}
