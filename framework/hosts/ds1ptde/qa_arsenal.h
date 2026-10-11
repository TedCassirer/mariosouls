#pragma once
// Explicit test setup only, invoked by the bounded test transport. Native item
// acquisition runs on the host update thread, never from a remote thread.
static void qaMeleeArsenal(){
 uintptr_t stats=chain(0x1378700,{0,8});wchar_t name[20]={};
 if(!stats||!get(stats+0xa0,name)||wcscmp(name,L"Mario QA")||currentMap()!=1801||
    !player||player!=lastPlayer||read<int>(player+0x2d4)<=0||characterOwned()||!harnessIdentity())return;
 auto observation=nativeInteractionState();if(!observation.valid||!observation.idle||observation.menu||observation.event||observation.ladder)return;
 // Only the known pristine level-six QA fixture can receive this one-time kit.
 if(read<int>(stats+0x88)!=6)return;
 for(int offset:{0x38,0x40,0x48,0x50,0x58,0x60,0x68,0x80})if(read<int>(stats+offset)!=11)return;
 const unsigned char signature[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x34,0x8b,0x4d,0x0c,0x53,0x8b,0x5d,0x08,0x56,0x83,0xc8,0xff,0x33,0xf6,0x81,0xf9,0,0,0,0x20};
 unsigned char actual[sizeof(signature)]={};
 if(!get(uintptr_t(0xc0b700),actual)||memcmp(actual,signature,sizeof(actual))){logf("QA arsenal rejected: acquisition signature");return;}
 // stdcall six arguments verified against the owned executable (ret 18h),
 // and the existing DS Gadget item-acquisition calling convention.
 using Acquire=int(__stdcall*)(uintptr_t,int,int,int,int,int);
 auto acquire=reinterpret_cast<Acquire>(0xc0b700);
 for(int weapon:{100000,201000,301000,400000,701000,801000}){
  auto item=equipmentItem(weapon);if(!item.valid||!item.melee){logf("QA arsenal rejected: item %d",weapon);return;}
 }
 for(int weapon:{100000,201000,301000,400000,701000,801000}){
  int result=acquire(stats+0x1b8,0,weapon,1,1,0);
  logf("QA arsenal native acquire weapon=%d result=%d",weapon,result);
 }
 put<int>(stats+0x50,16);put<int>(stats+0x58,13);put<int>(stats+0x88,13);
 logf("QA arsenal complete: Mario QA level13 STR16 DEX13; other attributes unchanged");
}
