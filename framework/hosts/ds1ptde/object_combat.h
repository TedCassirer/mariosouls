#pragma once
// Uses the same native damage entry builder as character hits, but passes a
// native map object handle rather than resolving a character script ID.
__attribute__((naked)) static int __cdecl worldDamageObject(int behavior,uintptr_t owner,uintptr_t target,const float* point){__asm__ volatile(
 "pushl %ebx\n pushl %esi\n pushl %edi\n movl 16(%esp),%ecx\n movl 20(%esp),%eax\n movl 24(%esp),%edx\n movl 28(%esp),%ebx\n pushl %ebx\n pushl %edx\n movl 0x137d6ac,%edx\n pushl %edx\n movl $0xdea8e0,%edi\n call *%edi\n popl %edi\n popl %esi\n popl %ebx\n ret\n");}
static void combatObjects(){
 if(!characterOwned()||!worldObjectDamageReady||!combatEnabled||!characterDamageReady||!characterWindow.active(state,*profile))return;
 if(harness.armed&&harness.suppressOutgoing)return;
 int move=characterWindow.move;if(move<0||(profile->attacks[move].flags&SC_ATTACK_NO_OBJECTS))return;
 uintptr_t manager=read<uintptr_t>(0x137d5f8),maps=read<uintptr_t>(manager+0x20);int count=read<int>(manager+0x1c);if(count<1||count>64||!maps)return;
 int serial=characterWindow.serial;
 Vec p=read<Vec>(posData+0x10),forward={sinf(state.faceAngle),0,cosf(state.faceAngle)};
 for(int m=0;m<count;m++){
  uintptr_t map=maps+m*0xec,objects=read<uintptr_t>(map+0x40);int n=read<int>(map+0x3c);if(!objects||n<1||n>10000)continue;
  for(int j=0;j<n;j++){
   uintptr_t obj=objects+j*0xc0;int handle=int(read<uint32_t>(obj+4));if(!characterWindow.canHit(handle,*profile)||read<int16_t>(obj+0x2a)<=0||!(read<uint8_t>(obj+0x2d)&0x40)||!read<uintptr_t>(obj+0x90))continue;
   Vec at={read<float>(obj+0x6c),read<float>(obj+0x7c)+.7f,read<float>(obj+0x8c)};
   for(const auto& c:worldContacts)if(c.kind==1&&c.owner==obj&&dot(c.point-p,c.point-p)<dot(at-p,at-p))at=c.point;Vec delta=at-(p+Vec{0,.7f,0});
   float radius=profile->attacks[move].rangeCm*.01f;
   if(fabsf(delta.y)>1.5f||delta.x*delta.x+delta.z*delta.z>radius*radius||(!(profile->attacks[move].flags&SC_ATTACK_RADIAL)&&dot(delta,forward)<0))continue;
   if(!lineClear(p+Vec{0,.7f,0},at)||!worldLineClear(p+Vec{0,.7f,0},at,obj))continue;
   characterWindow.hit.insert(handle);characterPacketMove=move;characterPacketPower=characterWindow.attackPower;characterPacketPoise=0;characterPacketBuilt=false;
   uint16_t objectPower=uint16_t(std::clamp(characterWindow.attackPower,1.f,1200.f));memcpy(characterAttacks[move]+0x62,&objectPower,2);
   alignas(16) float point[4]={at.x,at.y,at.z,1};int before=read<int16_t>(obj+0x2a);
   int result=worldDamageObject(990050+move,player,obj,point);characterPacketMove=-1;
   if(!characterPacketBuilt){characterDamageReady=false;if(harness.armed)harnessStop("native_object_damage_pipeline_unavailable");return;}
   feedbackTrack(obj,handle,before,serial,move,at,true);
   logf("WORLD Character object hit object=%08x index=%d hp_before=%d result=%d packet=%d move=%d serial=%d",(unsigned)obj,j,before,result,characterPacketBuilt,move,serial);
  }
 }
}
