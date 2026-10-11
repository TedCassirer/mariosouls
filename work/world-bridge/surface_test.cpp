#include <windows.h>
#include <cassert>
#include <cstdio>
#include "../libsm64/src/libsm64.h"
int main(){
 auto dll=LoadLibraryA("work/build/sm64.dll");assert(dll);
 auto load=(void(*)(const SM64Surface*,uint32_t))GetProcAddress(dll,"sm64_host_contacts_load");
 auto floor=(float(*)(float,float,float))GetProcAddress(dll,"sm64_surface_find_floor_height");assert(load&&floor);
 SM64Surface s[2]={};int vertices[2][3][3]={{{-50,100,-50},{-50,100,50},{50,100,50}},{{-50,100,-50},{50,100,50},{50,100,-50}}};
 for(int t=0;t<2;t++)for(int v=0;v<3;v++)for(int a=0;a<3;a++)s[t].vertices[v][a]=vertices[t][v][a];
 assert(floor(0,500,0)<-1000);load(s,2);assert(floor(0,500,0)==100);assert(floor(100,500,0)<-1000);
 for(auto& t:s)for(auto& v:t.vertices)v[1]=200;
 load(s,2);assert(floor(0,500,0)==200);load(nullptr,0);assert(floor(0,500,0)<-1000);
 for(int i=0;i<10000;i++){load(s,2);load(nullptr,0);}assert(floor(0,500,0)<-1000);
 puts("Host contact overlay: support, extent, replacement, removal and repeated refresh passed.");
}
