// Skip only the native startup request for the logo sequence.
// Adapted from the behavior identified in Rick (gibbed)'s PTDE No-Logo mod:
// https://blog.gib.me/2014/12/21/dark-souls-no-logo-hack-steam-build-2-0/
// All clocks, game time steps, movie files and normal menu transitions stay native.
using IntroPhaseFn=void(__thiscall*)(void*,uint32_t,uint32_t);
static IntroPhaseFn originalIntroPhase=nullptr;
static void __fastcall introPhase(void* context,void*,uint32_t a,uint32_t b){
 uintptr_t menu=read<uint32_t>(0x13786d0);
 bool ready=menu&&read<uint32_t>(menu+0x60)==0&&read<uint8_t>(0x1378622)==0;
 originalIntroPhase(context,a,b);
 if(ready&&read<uint32_t>(menu+0x10c)==1){put<uint32_t>(menu+0x10c,0);logf("startup logo request skipped");}
}
static bool skipIntroPresent(){return false;}
static void installStartupHooks(void**){
 if(!GetPrivateProfileIntA("MarioSouls","SkipIntro",1,ini))return;
 const unsigned char expected[]={0xa1,0xd0,0x86,0x37,0x01,0x83,0x78,0x60,0x00,0x75,0x17,0x80,0x3d,0x22,0x86,0x37,0x01,0x00,0x75,0x0e,0xba,0x01,0x00,0x00,0x00,0x89,0x90,0x0c,0x01,0x00,0x00,0x01,0x51,0x08,0xc2,0x08,0x00};
 unsigned char actual[sizeof(expected)]={};SIZE_T n=0;
 if(!ReadProcessMemory(GetCurrentProcess(),(void*)0xee5900,actual,sizeof(actual),&n)||n!=sizeof(actual)||memcmp(expected,actual,n)!=0){logf("startup phase signature unavailable; normal startup retained");return;}
 MH_STATUS result=MH_CreateHook((void*)0xee5900,(void*)introPhase,(void**)&originalIntroPhase);
 if(result==MH_OK)result=MH_EnableHook((void*)0xee5900);
 logf("startup phase hook status=%d",result);
}
