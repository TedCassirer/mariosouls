#pragma once
#include "../../core/starting_class.h"
static SC_ClassProfile classProfile{};
static bool classEnabled=false,classHooksReady=false;
static wchar_t className[64]={},classDescription[256]={};
static SCClass::Selection classSelection;
static int classPreview=-1;
static bool classPreviewActive=false;
static float classPreviewYaw=0;
static void classConfigure(){
 auto entry=(SC_GetClassProfile)GetProcAddress(adapterModule,"SoulsCharacterGetClassProfile");
 auto p=entry?entry(SC_CLASS_PROFILE_VERSION,sizeof(SC_ClassProfile)):nullptr;
 classEnabled=GetPrivateProfileIntA("Framework","StartingClass",1,ini)&&SCClass::valid(p);
 if(classEnabled){classProfile=*p;classEnabled=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,p->name,-1,className,64)>0&&MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,p->description,-1,classDescription,256)>0;}
}
static int classNativeSlot(){return 7;} // PTDE firecaster = Pyromancer.
static bool classCreationMenu(){
 uintptr_t menu=read<uint32_t>(0x13786d0);
 // Native new-character menu +94; no world actor may coexist with a preview.
 int phase=read<int>(menu+0x94);
 return classEnabled&&classHooksReady&&menu&&!chain(playerSlot,{0,4,0})&&(phase==2||phase==3||phase==5||phase==7);
}
static bool classSelected(){
 uintptr_t stats=chain(0x1378700,{0,8});uint8_t slot=255;
 return stats&&get(stats+0xc6,slot)&&slot==classNativeSlot();
}
extern "C" {void* classOriginalText=nullptr;void* classOriginalInit=nullptr;}
extern "C" __attribute__((used,noinline)) uintptr_t classText(int id,uintptr_t fmg){
 if(!classEnabled||!classHooksReady)return 0;
 uintptr_t repo=read<uint32_t>(0x13785dc);
 if(!repo||(fmg!=read<uint32_t>(repo+0x138)&&fmg!=read<uint32_t>(repo+0x1f0)))return 0;
 if(id==132020+classNativeSlot())return (uintptr_t)className;
 if(id==132320+classNativeSlot())return (uintptr_t)classDescription;
 return 0;
}
__attribute__((naked)) static void classTextHook(){__asm__ volatile(
 "pushfl\n pushal\n movl 40(%esp),%eax\n pushl %eax\n pushl %edx\n call _classText\n addl $8,%esp\n testl %eax,%eax\n jz 1f\n movl %eax,28(%esp)\n popal\n popfl\n ret\n 1: popal\n popfl\n jmp *_classOriginalText\n");}
// Native initializer returns an 8-byte {id,row}. Copy the user's own row, changing
// only equipment; no game data is shipped and no existing inventory is edited.
static unsigned char classRows[2][240];
static_assert(sizeof(classRows[0])==240,"PTDE CHARACTER_INIT_PARAM size");
extern "C" __attribute__((used,noinline)) uintptr_t classInitRow(int id,uintptr_t out){
 if(!classEnabled||!classHooksReady)return 0;
 int index=id==2000+classNativeSlot()?0:id==3000+classNativeSlot()?1:-1;
 if(index<0)return 0;
 uintptr_t row=equipmentParamRow(21,"CHARACTER_INIT_PARAM",id);
 if(!row||!get(row,classRows[index]))return 0;
 auto set=[&](int offset,int value){memcpy(classRows[index]+offset,&value,4);};
 set(0x10,1330000);set(0x14,-1);set(0x18,1406000);set(0x1c,-1);
 put<int>(out,id);put<uintptr_t>(out+4,(uintptr_t)classRows[index]);return (uintptr_t)classRows[index];
}
__attribute__((naked)) static void classInitHook(){__asm__ volatile(
 "pushfl\n pushal\n movl 40(%esp),%eax\n pushl %eax\n pushl %edi\n call _classInitRow\n addl $8,%esp\n testl %eax,%eax\n jz 1f\n movl %eax,28(%esp)\n popal\n popfl\n ret $4\n 1: popal\n popfl\n jmp *_classOriginalInit\n");}
static void classInstall(){
 if(!classEnabled)return;
 const unsigned char textSig[]={0x53,0x8b,0x5c,0x24,0x08,0x55,0x56};
 const unsigned char initSig[]={0x33,0xc0,0x53,0x8b,0x5c,0x24,0x08};
 unsigned char a[sizeof(textSig)]={},b[sizeof(initSig)]={};
 if(!get(uintptr_t(0xd175e0),a)||memcmp(a,textSig,sizeof(a))||!get(uintptr_t(0xd1fa40),b)||memcmp(b,initSig,sizeof(b))){logf("CLASS signatures refused");return;}
 auto x=MH_CreateHook((void*)0xd175e0,(void*)classTextHook,(void**)&classOriginalText);
 auto y=MH_CreateHook((void*)0xd1fa40,(void*)classInitHook,(void**)&classOriginalInit);
 if(x==MH_OK&&y==MH_OK){classHooksReady=MH_EnableHook((void*)0xd175e0)==MH_OK&&MH_EnableHook((void*)0xd1fa40)==MH_OK;}
 if(!classHooksReady){MH_DisableHook((void*)0xd175e0);MH_DisableHook((void*)0xd1fa40);}
 logf("CLASS native text/init hooks=%d/%d ready=%d",x,y,classHooksReady);
}
static void classPreviewRelease(){
 if(classPreview>=0){guest->destroy(classPreview);classPreview=-1;guest->surfaces(nullptr,0,0);geo.numTrianglesUsed=0;logf("CLASS preview released");}
 classPreviewActive=false;
}
static void classTick(){
 bool wanted=classCreationMenu()&&classSelected();
 if(!wanted)classPreviewRelease();
 if(wanted&&!characterOwned()&&character<0){
  if(classPreview<0){
   // Preview-only floor. No world physics, health, input capture or native actor.
   SC_Surface floor={};floor.vertices[0][0]=-1000;floor.vertices[0][2]=-1000;floor.vertices[1][2]=1000;floor.vertices[2][0]=1000;floor.vertices[2][2]=-1000;
   guest->surfaces(&floor,1,0);classPreview=guest->create(0,0,0,0);logf("CLASS preview created=%d",classPreview);
  }
  static DWORD last=0;DWORD now=GetTickCount();
  if(classPreview>=0&&now-last>=33){last=now;
   uintptr_t viewer=chain(0x13783c0,{0,0x6c0,0xc});float angle=read<float>(viewer+0x1b4);
   classPreviewYaw=classProfile.previewYaw+(isfinite(angle)?angle:0);
   SC_Pose pose={};pose.phase=-1;pose.yaw=classPreviewYaw;
   classPreviewActive=guestPose(classPreview,&pose,&state,&geo);
   if(!classPreviewActive)classPreviewRelease();
  }
 }
 if(classEnabled&&classHooksReady&&classSelection.observe(player,classSelected())){
  characterSession.request(true);logf("CLASS native saved class selected character=%s",profile->id);
 }
}
