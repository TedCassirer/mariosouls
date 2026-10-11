#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../sdk/character_api.h"
#include "../core/validation.h"
static void log(const char*s){printf("adapter: %s\n",s);}
int main(int argc,char**argv){
 if(argc<2)return 2;HMODULE dll=LoadLibraryA(argv[1]);if(!dll){printf("LoadLibrary failed %lu\n",GetLastError());return 3;}
 auto entry=(SC_GetAPI)GetProcAddress(dll,"SoulsCharacterGetAPI");assert(entry);assert(!entry(99,sizeof(SC_CharacterAPI)));assert(!entry(SC_ABI_VERSION,0));
 const auto*a=entry(SC_ABI_VERSION,sizeof(SC_CharacterAPI));assert(SC::api(a));const auto*d=a->describe();assert(SC::descriptor(d));
 std::vector<uint8_t> atlas(d->textureWidth*d->textureHeight*4);
 SC_Init init={sizeof(SC_Init),argc>2?argv[2]:"",argc>3?argv[3]:"",log};SC_Texture tex={atlas.data(),uint32_t(atlas.size())};
 assert(a->initialize(&init,&tex));assert(!a->initialize(&init,&tex));
 SC_Surface floor[2]={{{{-16000,0,-16000},{-16000,0,16000},{16000,0,16000}},0},{{{-16000,0,-16000},{16000,0,16000},{16000,0,-16000}},0}};
 a->surfaces(floor,2,0);assert(fabsf(a->floorHeight(0,100,0))<.01f);
 int id=a->create(0,30,0,0);assert(id>=0);assert(a->create(0,30,0,0)<0);
 std::vector<float> pos(d->maxTriangles*9),normal(pos.size()),color(pos.size()),uv(d->maxTriangles*6);
 SC_Mesh m={pos.data(),normal.data(),color.data(),uv.data(),d->maxTriangles,0};SC_State state{};SC_Input input={0,1,0,0,0,0,0,0};
 auto step=[&](){assert(a->step(id,&input,&state,&m));assert(SC::frame(state,m,*d));assert(m.numTrianglesUsed);};
 for(int i=0;i<30;i++)step();assert(state.flags&SC_GROUNDED);float ground=state.position[1],top=ground;
 for(int i=0;i<70;i++){input.jump=i<8;step();top=std::max(top,state.position[1]);}assert(top-ground>10);assert(fabsf(state.position[1]-ground)<5);
 printf("%s: mesh=%u jump=%.2f cm\n",d->id,m.numTrianglesUsed,top-ground);
 input.jump=0;input.moveY=1;for(int i=0;i<45;i++)step();assert(fabsf(state.position[2])>50);
 for(int kind=SC_POSE_NEUTRAL;kind<=SC_POSE_TALK;kind++)for(int frame=0;frame<20;frame++){
  SC_Pose p={uint32_t(kind),{100,50,200},.75f,frame/19.f,float(frame*3)};assert(a->pose(id,&p,&state,&m));assert(SC::frame(state,m,*d));assert(fabsf(state.position[1]-50)<.01f);
 }
 a->resume(id,0,0,0,0);input={0,1,0,0,0,0,0,0};step();input.jump=1;step();assert(state.position[1]>0);input.jump=0;
 int16_t tiny[2]={123,456};assert(a->audio(0,1100,tiny,1)==0);assert(tiny[0]==123&&tiny[1]==456);
 int16_t pcm[2176];assert(a->audio(0,1100,pcm,1088)<=1088);
 a->nativeDamage(id,1);step();a->nativeDamage(id,0);
 SC_Mesh tooSmall=m;tooSmall.capacityTriangles=0;assert(!a->step(id,&input,&state,&tooSmall));
 a->destroy(id);assert(!a->step(id,&input,&state,&m));id=a->create(0,30,0,0);assert(id>=0);for(int i=0;i<30;i++)step();a->destroy(id);a->shutdown();FreeLibrary(dll);
 puts("PASS adapter contract: version rejection, initialization, geometry, movement, jump, poses, same-instance resume, audio bounds, damage callback, destroy/recreate");
}
