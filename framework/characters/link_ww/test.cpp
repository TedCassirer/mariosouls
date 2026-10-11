#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <vector>
#include <cstdio>
#include <cmath>
#include "character_api.h"
int main(int argc,char**argv){
 assert(argc==3);auto dll=LoadLibraryA(argv[1]);assert(dll);auto get=(SC_GetAPI)GetProcAddress(dll,"SoulsCharacterGetAPI");assert(get);auto a=get(SC_ABI_VERSION,sizeof(SC_CharacterAPI));auto d=a->describe();
 std::vector<uint8_t> tex(d->textureWidth*d->textureHeight*4);SC_Texture t={tex.data(),unsigned(tex.size())};SC_Init init={sizeof(init),argv[2],"",nullptr};assert(a->initialize(&init,&t));
 SC_Surface floor[]={{{{-10000,0,-10000},{-10000,0,10000},{10000,0,10000}},0},{{{-10000,0,-10000},{10000,0,10000},{10000,0,-10000}},0}};a->surfaces(floor,2,0);int id=a->create(0,0,0,0);assert(id>=0);
 std::vector<float> p(d->maxTriangles*9),n(p.size()),c(p.size()),u(d->maxTriangles*6);SC_Mesh m={p.data(),n.data(),c.data(),u.data(),d->maxTriangles,0};SC_State s{};SC_Input in={0,1,0,0,0,0,0,0};
 auto step=[&](){assert(a->step(id,&in,&s,&m));};
 // Holding attack must give one swing, never unlimited repeat/contact damage.
 int active=0;in.attack=1;
 for(int i=0;i<90;i++){step();if(s.flags&SC_ATTACK_ACTIVE){active++;assert(s.attackId==0);assert(a->probeContact(id,s.position[0],0,s.position[2]+90)&SC_CONTACT_ATTACK);assert(!a->probeContact(id,s.position[0],0,s.position[2]-90));}}
 assert(active==7&&s.attackId==-1);
 a->resume(id,0,0,0,0);in.attack=0;step();bool seen[3]={};
 for(int i=0;i<80;i++){in.attack=(i==0||i==10||i==32);step();if(s.flags&SC_ATTACK_ACTIVE){assert(s.attackId>=0&&s.attackId<3);seen[s.attackId]=true;}}
 assert(seen[0]&&seen[1]&&seen[2]);
 a->resume(id,0,0,0,0);in={0,1,0,0,0,1,1,0};active=0;
 for(int i=0;i<40;i++){step();if(s.flags&SC_ATTACK_ACTIVE){active++;assert(s.attackId==3);assert(a->probeContact(id,s.position[0],0,s.position[2]-90)&SC_CONTACT_ATTACK);}}
 assert(active==13&&s.attackId==-1);
 a->resume(id,0,0,0,0);in={0,1,0,1,0,0,1,0};step();assert(s.rawAction==2);assert(!(s.flags&SC_ATTACK_ACTIVE));a->nativeDamage(id,1);in.crouch=0;step();assert(s.rawAction==3&&s.attackId==-1);assert(!a->probeContact(id,0,0,0));
 // Fixed wall and ceiling match the declared small native body.
 SC_Surface tunnel[]={floor[0],floor[1],{{{-1000,0,200},{-1000,500,200},{1000,500,200}},0},{{{-1000,0,200},{1000,500,200},{1000,0,200}},0},{{{-1000,140,-1000},{1000,140,1000},{-1000,140,1000}},0},{{{-1000,140,-1000},{1000,140,-1000},{1000,140,1000}},0}};
 a->surfaces(tunnel,6,0);a->resume(id,0,0,0,0);in={0,1,0,1,0,0,0,0};for(int i=0;i<80;i++)step();assert(fabsf(s.position[2]-178)<1);
 a->resume(id,0,0,0,0);in={0,1,0,0,0,0,0,0};step();float peak=0;
 for(int i=0;i<60;i++){in.jump=i<4;step();peak=std::max(peak,s.position[1]);}assert(peak<=26&&s.flags&SC_GROUNDED);
 a->destroy(id);a->shutdown();FreeLibrary(dll);puts("PASS Link: single held swing, three distinct buffered attacks, radial spin, damage interrupts roll, actual wall/ceiling collision");
}
