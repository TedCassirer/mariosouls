#undef NDEBUG
#include "../core/kinematic.h"
#include <cassert>
#include <cstdio>
int main(){
 SCKinematic::World w;
 SC_Surface geometry[]={
  {{{-1000,0,-1000},{-1000,0,1000},{1000,0,1000}},0},{{{-1000,0,-1000},{1000,0,1000},{1000,0,-1000}},0},
  {{{-1000,140,-1000},{1000,140,1000},{-1000,140,1000}},0},{{{-1000,140,-1000},{1000,140,-1000},{1000,140,1000}},0},
  {{{-1000,0,200},{-1000,500,200},{1000,500,200}},0},{{{-1000,0,200},{1000,500,200},{1000,0,200}},0}};
 w.surfaces(geometry,6,false);assert(w.floor(0,20,0)==0);assert(w.floor(2000,20,0)==-100000);assert(w.ceiling(0,115,0)==140);
 float p[]={0,0,0};w.horizontal(p,0,300,22,115);assert(fabsf(p[2]-178)<.1f);
 p[2]=300;w.horizontal(p,0,-200,22,115);assert(fabsf(p[2]-222)<.1f);
 SC_Surface platform[]={{{{-100,50,-100},{-100,50,100},{100,50,100}},0},{{{-100,50,-100},{100,50,100},{100,50,-100}},0}};
 w.surfaces(platform,2,true);assert(w.floor(0,70,0)==50);assert(w.floor(0,20,0)==0);w.surfaces(nullptr,0,true);assert(w.floor(0,70,0)==0);
 w.clear();assert(w.floor(0,100,0)==-100000);puts("PASS neutral collision: stacked floors, missing floor, ceiling, swept walls both sides, dynamic removal");
}
