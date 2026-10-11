#pragma once
// Verified PTDE CamSfx render submission, __thiscall without arguments.
// +10 is its camera-to-world matrix; +50..5c is projection. The native function
// submits the inverse transform to the renderer. Override only during that
// call, then restore the follow camera's simulation matrix immediately.
static SRWLOCK aimCameraLock=SRWLOCK_INIT;
struct AimCameraFrame {uintptr_t camera=0,actor=0;Vec eye{},forward{};DWORD time=0;bool active=false;};
static AimCameraFrame aimCameraRequest,aimCameraRendered;
static bool aimCameraReady=false;
using AimCameraFn=void(__thiscall*)(void*);
static AimCameraFn aimCameraOriginal=nullptr;
static void aimCameraSet(bool active,Vec eye={},Vec forward={}){
 AcquireSRWLockExclusive(&aimCameraLock);
 aimCameraRequest={cam,player,eye,forward,GetTickCount(),active&&aimCameraReady};
 ReleaseSRWLockExclusive(&aimCameraLock);
}
static void __fastcall aimCameraSubmit(void*self,void*){
 uintptr_t pointer=(uintptr_t)self;float saved[16]={};AimCameraFrame frame;
 AcquireSRWLockShared(&aimCameraLock);frame=aimCameraRequest;ReleaseSRWLockShared(&aimCameraLock);
 bool selected=pointer==chain(cameraSlot,{0,0x3c,0x60});
 bool replaced=selected&&frame.active&&frame.camera==pointer&&frame.actor==chain(playerSlot,{0,4,0})&&read<int>(frame.actor+0x2d4)>0&&DWORD(GetTickCount()-frame.time)<150&&get(pointer+0x10,saved);
 if(replaced){
  Vec z=unit(frame.forward),x=unit(cross({0,1,0},z)),y=cross(z,x);
  float transform[16]={x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,frame.eye.x,frame.eye.y,frame.eye.z,1};
  replaced=put(pointer+0x10,transform);
 }
 if(selected){
  AimCameraFrame rendered={pointer,frame.actor,read<Vec>(pointer+0x40),read<Vec>(pointer+0x30),GetTickCount(),true};
  AcquireSRWLockExclusive(&aimCameraLock);aimCameraRendered=rendered;ReleaseSRWLockExclusive(&aimCameraLock);
 }
 aimCameraOriginal(self);
 if(replaced)put(pointer+0x10,saved);
}
static bool aimCameraInstall(){
 if(aimCameraReady)return true;
 const unsigned char expected[]={0x56,0x8b,0xf1,0x8b,0x4e,0x04,0x85,0xc9,0x74,0x31,0xd9,0x46,0x5c};unsigned char bytes[sizeof(expected)]={};
 if(!get(uintptr_t(0xff7460),bytes)||memcmp(bytes,expected,sizeof(bytes)))return false;
 auto status=MH_CreateHook((void*)0xff7460,(void*)aimCameraSubmit,(void**)&aimCameraOriginal);
 if(status==MH_OK)status=MH_EnableHook((void*)0xff7460);
 aimCameraReady=status==MH_OK;logf("CAMERA scripted render submission hook=%d",aimCameraReady);return aimCameraReady;
}
static Vec cameraFrameVector(bool forward){
 AcquireSRWLockShared(&aimCameraLock);auto frame=aimCameraRendered;ReleaseSRWLockShared(&aimCameraLock);
 if(frame.active&&frame.camera==cam&&DWORD(GetTickCount()-frame.time)<150)return forward?frame.forward:frame.eye;
 return read<Vec>(cam+(forward?0x30:0x40));
}
