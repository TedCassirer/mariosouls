#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
#include "../../core/validation.h"
#include "../../../work/libsm64/src/libsm64.h"
int main(int argc,char**argv){
 setvbuf(stdout,nullptr,_IONBF,0);assert(argc==4);auto dll=LoadLibraryA(argv[1]);assert(dll);
 auto entry=(SC_GetAPI)GetProcAddress(dll,"SoulsCharacterGetAPI");assert(entry);
 auto api=entry(SC_ABI_VERSION,sizeof(SC_CharacterAPI));assert(SC::api(api));auto d=api->describe();
 std::vector<uint8_t> pixels(d->textureWidth*d->textureHeight*4);SC_Texture texture={pixels.data(),uint32_t(pixels.size())};
 SC_Init init={sizeof(init),argv[2],argv[3],nullptr};assert(api->initialize(&init,&texture));
 std::vector<float> pos(d->maxTriangles*9),normal(pos.size()),color(pos.size()),uv(d->maxTriangles*6);
 SC_Mesh mesh={pos.data(),normal.data(),color.data(),uv.data(),d->maxTriangles,0};SC_State state{};
 auto runtime=LoadLibraryA(argv[3]);assert(runtime);
 auto rawSurfaces=(decltype(&sm64_static_surfaces_load))GetProcAddress(runtime,"sm64_static_surfaces_load");assert(rawSurfaces);
 auto ramp=[&](int degrees,int type){
  int height=int(lroundf(tanf(degrees*3.14159265f/180)*8000));
  SC_Surface floor[2]={{{{-8000,-height,-8000},{-8000,height,8000},{8000,height,8000}},uint32_t(type)},
                       {{{-8000,-height,-8000},{8000,height,8000},{8000,-height,-8000}},uint32_t(type)}};
  api->surfaces(floor,2,0);
  if(type){ // Direct guest fixture: SDK materials currently map to ordinary terrain.
   SM64Surface guestFloor[2]={};
   for(int i=0;i<2;i++){memcpy(guestFloor[i].vertices,floor[i].vertices,sizeof(floor[i].vertices));guestFloor[i].type=type;}
   rawSurfaces(guestFloor,2);
  }
 };
 auto tick=[&](int id,SC_Input input){api->nativeDamage(id,0);assert(api->step(id,&input,&state,&mesh)&&SC::frame(state,mesh,*d));};
 SC_Input idle{};idle.camLookZ=1;
 for(int slope:{0,10,20,30,40}){
  ramp(slope,0);int id=api->create(0,0,0,0);assert(id>=0);
  for(int i=0;i<300;i++)tick(id,idle);
  float drift=hypotf(state.position[0],state.position[2]);
  printf("ordinary slope %d: idle drift %.2f cm action=%08x\n",slope,drift,state.rawAction);
  assert(drift<3&&fabsf(state.forwardSpeed)<1);
  // Running in both directions must recover to rest rather than drift forever.
  for(int direction:{-1,1}){
   api->resume(id,0,0,0,0);SC_Input run=idle;run.moveY=float(direction);
   for(int i=0;i<35;i++)tick(id,run);
   assert(fabsf(state.position[2])>100);
   for(int i=0;i<90;i++)tick(id,idle);
   float stopped=state.position[2];for(int i=0;i<90;i++)tick(id,idle);
   printf("  direction %d: release drift %.2f cm speed %.2f\n",direction,fabsf(state.position[2]-stopped),state.forwardSpeed);
   assert(fabsf(state.position[2]-stopped)<3&&fabsf(state.forwardSpeed)<1);
  }
  api->destroy(id);
 }
 // Genuine steep terrain and explicit ice must not become adhesive.
 for(auto pair:{std::pair<int,int>{50,0},{20,0x2e}}){
  ramp(pair.first,pair.second);int id=api->create(0,0,0,0);assert(id>=0);
  for(int i=0;i<45;i++)tick(id,idle);
  printf("steep/ice slope %d type=%d: travel %.2f cm\n",pair.first,pair.second,fabsf(state.position[2]));
  assert(fabsf(state.position[2])>50);api->destroy(id);
 }
 api->shutdown();FreeLibrary(runtime);FreeLibrary(dll);
 puts("PASS real Mario slope traction: idle, uphill/downhill release, steep slides and explicit ice");
}
