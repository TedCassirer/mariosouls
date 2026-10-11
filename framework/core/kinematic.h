#pragma once
// Guest-neutral triangle queries for guest gravity and static collision.
// The host separately resolves native actors, doors and contact bodies.
#include "../sdk/character_api.h"
#include <vector>
#include <cmath>
#include <algorithm>
namespace SCKinematic {
struct V {float x,y,z;V operator+(V b)const{return{x+b.x,y+b.y,z+b.z};}V operator-(V b)const{return{x-b.x,y-b.y,z-b.z};}V operator*(float f)const{return{x*f,y*f,z*f};}};
inline float dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V cross(V a,V b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
struct Triangle {V a,b,c,n;float minX,maxX,minZ,maxZ,minY,maxY;};
class World {
 std::vector<Triangle> fixed,dynamic;
 template<class Fn>void each(Fn fn)const{for(const auto&t:fixed)fn(t);for(const auto&t:dynamic)fn(t);}
 static bool within(const Triangle&t,V p){return dot(cross(t.b-t.a,p-t.a),t.n)>=-.01f&&dot(cross(t.c-t.b,p-t.b),t.n)>=-.01f&&dot(cross(t.a-t.c,p-t.c),t.n)>=-.01f;}
public:
 void clear(){fixed.clear();dynamic.clear();}
 void surfaces(const SC_Surface*s,uint32_t count,bool moving){
  auto&out=moving?dynamic:fixed;out.clear();if(!s||count>2000000)return;out.reserve(count);
  for(unsigned i=0;i<count;i++){
   auto v=[&](int j){return V{float(s[i].vertices[j][0]),float(s[i].vertices[j][1]),float(s[i].vertices[j][2])};};
   Triangle t{};t.a=v(0);t.b=v(1);t.c=v(2);t.n=cross(t.b-t.a,t.c-t.a);float len=sqrtf(dot(t.n,t.n));if(len<.01f)continue;t.n=t.n*(1/len);
   t.minX=std::min({t.a.x,t.b.x,t.c.x});t.maxX=std::max({t.a.x,t.b.x,t.c.x});t.minZ=std::min({t.a.z,t.b.z,t.c.z});t.maxZ=std::max({t.a.z,t.b.z,t.c.z});t.minY=std::min({t.a.y,t.b.y,t.c.y});t.maxY=std::max({t.a.y,t.b.y,t.c.y});out.push_back(t);
  }
 }
 float floor(float x,float y,float z)const{
  float best=-100000;
  each([&](const Triangle&t){if(t.n.y<.35f||x<t.minX||x>t.maxX||z<t.minZ||z>t.maxZ)return;
   float h=t.a.y-(t.n.x*(x-t.a.x)+t.n.z*(z-t.a.z))/t.n.y;
   if(h<=y+.01f&&h>best&&within(t,{x,h,z}))best=h;
  });return best;
 }
 float ceiling(float x,float y,float z)const{
  float best=100000;
  each([&](const Triangle&t){if(t.n.y>-.35f||x<t.minX||x>t.maxX||z<t.minZ||z>t.maxZ)return;
   float h=t.a.y-(t.n.x*(x-t.a.x)+t.n.z*(z-t.a.z))/t.n.y;
   if(h>=y-.01f&&h<best&&within(t,{x,h,z}))best=h;
  });return best;
 }
 void horizontal(float*p,float dx,float dz,float radius,float height)const{
  int steps=std::max(1,int(ceilf(sqrtf(dx*dx+dz*dz)/std::max(4.f,radius*.5f))));steps=std::min(steps,32);
  for(int step=0;step<steps;step++){
   V previous={p[0],p[1],p[2]};p[0]+=dx/steps;p[2]+=dz/steps;
   for(int pass=0;pass<2;pass++)each([&](const Triangle&t){
    if(fabsf(t.n.y)>.65f||p[0]+radius<t.minX||p[0]-radius>t.maxX||p[2]+radius<t.minZ||p[2]-radius>t.maxZ||p[1]+height<t.minY||p[1]>t.maxY)return;
    for(float y:{radius,height*.5f,height-radius}){
     V point={p[0],p[1]+y,p[2]};float d=dot(point-t.a,t.n);if(fabsf(d)>=radius)continue;
     V projected=point-t.n*d;if(!within(t,projected))continue;
     float old=dot(V{previous.x,previous.y+y,previous.z}-t.a,t.n);float side=old<0?-1.f:1.f;
     float horiz=t.n.x*t.n.x+t.n.z*t.n.z;if(horiz<.01f)continue;
     float push=(side*radius-d)/horiz;p[0]+=t.n.x*push;p[2]+=t.n.z*push;
    }
   });
  }
 }
};
}
