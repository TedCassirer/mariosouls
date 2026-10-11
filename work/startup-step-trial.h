// Logo identification follows Peter Thoman's DSfix texture signatures.
// Counter acceleration is active only while a recognized startup logo is drawn.
#include "startup_hash.h"
static bool skipIntro=false,introActive=false,introFinished=false;
static DWORD introStarted=0;
static unsigned introFrames=0;
static IDirect3DBaseTexture9* introTextures[3]={};
static float introStep=1.f/30.f;
static uintptr_t introStepAddress=0;
static void stopIntro(){if(introActive){if(introStepAddress)put(introStepAddress,introStep);logf("intro skip finished: %lu ms, %u frames",GetTickCount()-introStarted,introFrames);}introActive=false;introFinished=true;}
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
 if(skipIntro&&!introFinished&&t){if(introActive&&t==introTextures[2])stopIntro();else if(!introActive&&(t==introTextures[0]||t==introTextures[1])){introActive=true;introStarted=GetTickCount();if(introStepAddress){introStep=read<float>(introStepAddress);put(introStepAddress,.5f);}logf("intro skip started");}}
 return realSetTexture(d,stage,t);
}
static bool skipIntroPresent(){if(introActive&&(player||GetTickCount()-introStarted>6000||++introFrames>600))stopIntro();return false;}
static void installStartupHooks(void**table){
 skipIntro=GetPrivateProfileIntA("MarioSouls","SkipIntro",1,ini)!=0;if(!skipIntro)return;
 // This constant is unique in the supported executable, from DSfix's time-step signature.
 const unsigned char expected[]={0x00,0x80,0x26,0x44,0x00,0x00,0x94,0x44,0x00,0x00,0x58,0x42,0x00,0x00,0xc0,0x42,0x89,0x88,0x08,0x3d,0x00,0x00,0xa0,0x44};
 unsigned char actual[sizeof(expected)]={};SIZE_T n=0;if(ReadProcessMemory(GetCurrentProcess(),(void*)0x11e7e80,actual,sizeof(actual),&n)&&n==sizeof(actual)&&memcmp(expected,actual,n)==0)introStepAddress=0x11e7e90;
 if(!introStepAddress){skipIntro=false;logf("startup step signature unavailable");return;}
 HMODULE dx=GetModuleHandleA("d3dx9_43.dll");void*a=(void*)GetProcAddress(dx,"D3DXCreateTextureFromFileInMemory");void*b=(void*)GetProcAddress(dx,"D3DXCreateTextureFromFileInMemoryEx");
 if(a&&MH_CreateHook(a,(void*)startupTextureMemory,(void**)&realTextureMemory)==MH_OK)MH_EnableHook(a);
 if(b&&MH_CreateHook(b,(void*)startupTextureMemoryEx,(void**)&realTextureMemoryEx)==MH_OK)MH_EnableHook(b);
 if(MH_CreateHook(table[65],(void*)startupSetTexture,(void**)&realSetTexture)==MH_OK)MH_EnableHook(table[65]);
 logf("startup logo step hooks installed");
}
