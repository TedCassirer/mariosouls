#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "../../work/minhook/include/MinHook.h"
static DWORD testClock=1000;
static DWORD testNow(){return testClock;}
#define GetTickCount() testNow()
struct Vec{float x,y,z;};
static float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static Vec cross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static Vec unit(Vec v){float n=sqrtf(dot(v,v));return n>.0001f?Vec{v.x/n,v.y/n,v.z/n}:Vec{0,0,1};}
static uintptr_t cam=0,player=100,cameraSlot=1,playerSlot=2;
static int hp=100;static volatile LONG actionAimUntil=0;
static void logf(const char*,...){}
static uintptr_t chain(uintptr_t slot,std::initializer_list<unsigned>){return slot==cameraSlot?cam:player;}
template<class T>static T read(uintptr_t p){T value{};if(p==player+0x2d4){memcpy(&value,&hp,sizeof(value));return value;}memcpy(&value,(void*)p,sizeof(value));return value;}
template<class T>static bool get(uintptr_t p,T&v){memcpy(&v,(void*)p,sizeof(v));return true;}
template<class T>static bool put(uintptr_t p,const T&v){memcpy((void*)p,&v,sizeof(v));return true;}
#include "../hosts/ds1ptde/aim_camera.h"
// Synthetic getter with the verified register contract; no game binary/code.
__attribute__((naked)) static void fakeGetter(){__asm__ volatile(
 "movups 16(%ecx),%xmm0\n movups %xmm0,0(%eax)\n"
 "movups 32(%ecx),%xmm0\n movups %xmm0,16(%eax)\n"
 "movups 48(%ecx),%xmm0\n movups %xmm0,32(%eax)\n"
 "movups 64(%ecx),%xmm0\n movups %xmm0,48(%eax)\n addl $64,%ecx\n ret\n");}
static void invoke(uintptr_t source,float*out){
 uintptr_t a=(uintptr_t)out,c=source;
 __asm__ volatile("call *%%edx" : "+a"(a),"+c"(c) : "d"(aimCameraCopyHook) : "memory","cc","xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7");
 assert(a==(uintptr_t)out&&c==source+64);
}
int main(){
 float source[20]={},other[20]={},out[16]={};source[4]=source[9]=source[14]=source[19]=1;source[16]=10;source[17]=20;source[18]=30;
 memcpy(other,source,sizeof(source));cam=(uintptr_t)source;aimCameraOriginalCopy=(void*)fakeGetter;aimCameraReady=true;
 invoke(cam,out);assert(!memcmp(out,source+4,sizeof(out)));
 aimCameraSet(true,{1,2,3},{1,0,0});invoke(cam,out);
 assert(out[12]==1&&out[13]==2&&out[14]==3&&out[8]==1&&out[10]==0);
 assert(source[16]==10&&source[17]==20&&source[18]==30); // no mutation of follow camera
 assert(cameraFrameVector(false).x==1&&cameraFrameVector(true).x==1);
 invoke((uintptr_t)other,out);assert(!memcmp(out,other+4,sizeof(out))); // other views untouched
 testClock+=150;invoke(cam,out);assert(!memcmp(out,source+4,sizeof(out)));
 aimCameraSet(true,{1,2,3},{0,0,1});hp=0;invoke(cam,out);assert(!memcmp(out,source+4,sizeof(out)));hp=100;
 aimCameraSet(true,{1,2,3},{0,0,1});player=200;invoke(cam,out);assert(!memcmp(out,source+4,sizeof(out)));
 aimCameraSet(true,{1,2,3},{0,0,1});aimCameraSet(false);invoke(cam,out);assert(!memcmp(out,source+4,sizeof(out))&&!actionAimUntil);
 for(float yaw:{-.8f,0.f,.8f}){float pitch=.4f;Vec f={sinf(yaw)*cosf(pitch),sinf(pitch),cosf(yaw)*cosf(pitch)};
  aimCameraSet(true,{1,2,3},f);invoke(cam,out);assert(fabsf(out[8]-f.x)<.0001f&&out[12]==1&&out[13]==2&&out[14]==3);
 }
 puts("PASS camera export hook: register contract, fixed pivot, changing aim, original preserved, other cameras, expiry, death, actor change and cancel");
}
