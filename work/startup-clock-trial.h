// Logo identification follows Peter Thoman's DSfix texture signatures.
// Counter acceleration is active only while a recognized startup logo is drawn.
#include "startup_hash.h"
static bool skipIntro=false,introActive=false,introFinished=false;
static DWORD introStarted=0,introThread=0;
static unsigned introFrames=0;
static LONGLONG introOffset=0,introFrequency=0;
static DWORD introMillis=0;
using MillisFn=DWORD(WINAPI*)();
static MillisFn realMillis=timeGetTime;
static DWORD WINAPI introTimeGetTime(){return realMillis()+introMillis;}
static DWORD WINAPI introGetTickCount(){return GetTickCount()+introMillis;}
static IDirect3DBaseTexture9* introTextures[3]={};
using CounterFn=BOOL(WINAPI*)(LARGE_INTEGER*);
static CounterFn realCounter=QueryPerformanceCounter;
static unsigned counterCalls=0;
static BOOL WINAPI introCounter(LARGE_INTEGER*p){BOOL result=realCounter(p);p->QuadPart+=introOffset;return result;}
static void stopIntro(){if(introActive)logf("intro skip finished: %lu ms, %u frames, %u timer calls",GetTickCount()-introStarted,introFrames,counterCalls);introActive=false;introFinished=true;}
static void registerStartupTexture(const void*data,UINT size,IDirect3DTexture9*t){
 if(!t||!skipIntro||introFinished||size>16000000)return;
 uint32_t hash=SuperFastHash((const char*)data,size);
 int slot=hash==0x2fba57c1?0:hash==0x8618c56e?1:hash==0x3f28b833?2:-1;
 if(slot>=0){introTextures[slot]=t;logf("startup texture %d detected",slot);}
}
using TextureMemoryFn=HRESULT(WINAPI*)(IDirect3DDevice9*,const void*,UINT,IDirect3DTexture9**);
static TextureMemoryFn realTextureMemory;
static HRESULT WINAPI startupTextureMemory(IDirect3DDevice9*d,const void*p,UINT n,IDirect3DTexture9**out){auto r=realTextureMemory(d,p,n,out);if(SUCCEEDED(r))registerStartupTexture(p,n,*out);return r;}
using TextureMemoryExFn=HRESULT(WINAPI*)(IDirect3DDevice9*,const void*,UINT,UINT,UINT,UINT,DWORD,D3DFORMAT,D3DPOOL,DWORD,DWORD,D3DCOLOR,void*,PALETTEENTRY*,IDirect3DTexture9**);
static TextureMemoryExFn realTextureMemoryEx;
static HRESULT WINAPI startupTextureMemoryEx(IDirect3DDevice9*d,const void*p,UINT n,UINT w,UINT h,UINT levels,DWORD usage,D3DFORMAT format,D3DPOOL pool,DWORD filter,DWORD mip,D3DCOLOR color,void*info,PALETTEENTRY*palette,IDirect3DTexture9**out){auto r=realTextureMemoryEx(d,p,n,w,h,levels,usage,format,pool,filter,mip,color,info,palette,out);if(SUCCEEDED(r))registerStartupTexture(p,n,*out);return r;}
using SetTextureFn=HRESULT(WINAPI*)(IDirect3DDevice9*,DWORD,IDirect3DBaseTexture9*);
static SetTextureFn realSetTexture;
static HRESULT WINAPI startupSetTexture(IDirect3DDevice9*d,DWORD stage,IDirect3DBaseTexture9*t){
 if(skipIntro&&!introFinished&&t){if(introActive&&t==introTextures[2])stopIntro();else if(!introActive&&(t==introTextures[0]||t==introTextures[1])){introActive=true;introStarted=GetTickCount();introThread=GetCurrentThreadId();logf("intro skip started");}}
 return realSetTexture(d,stage,t);
}
static bool skipIntroPresent(){if(!introActive)return false;if(player||GetTickCount()-introStarted>8000||++introFrames>1500){stopIntro();return false;}introMillis+=33;introOffset+=introFrequency*33/1000;return true;}
static void installStartupHooks(void**table){
 skipIntro=GetPrivateProfileIntA("MarioSouls","SkipIntro",1,ini)!=0;if(!skipIntro)return;
 LARGE_INTEGER f;QueryPerformanceFrequency(&f);introFrequency=f.QuadPart;
 void*counter=(void*)QueryPerformanceCounter;if(MH_CreateHook(counter,(void*)introCounter,(void**)&realCounter)!=MH_OK){skipIntro=false;logf("startup clock hook unavailable");return;}MH_EnableHook(counter);
 void*millis=(void*)timeGetTime;if(MH_CreateHook(millis,(void*)introTimeGetTime,(void**)&realMillis)==MH_OK)MH_EnableHook(millis);
 DWORD old=0;DWORD*hostTicks=(DWORD*)0x10cc134;if(VirtualProtect(hostTicks,4,PAGE_READWRITE,&old)){*hostTicks=(DWORD)(uintptr_t)introGetTickCount;VirtualProtect(hostTicks,4,old,&old);}
 HMODULE dx=GetModuleHandleA("d3dx9_43.dll");void*a=(void*)GetProcAddress(dx,"D3DXCreateTextureFromFileInMemory");void*b=(void*)GetProcAddress(dx,"D3DXCreateTextureFromFileInMemoryEx");
 if(a&&MH_CreateHook(a,(void*)startupTextureMemory,(void**)&realTextureMemory)==MH_OK)MH_EnableHook(a);
 if(b&&MH_CreateHook(b,(void*)startupTextureMemoryEx,(void**)&realTextureMemoryEx)==MH_OK)MH_EnableHook(b);
 if(MH_CreateHook(table[65],(void*)startupSetTexture,(void**)&realSetTexture)==MH_OK)MH_EnableHook(table[65]);
 logf("startup skip hooks installed");
}
