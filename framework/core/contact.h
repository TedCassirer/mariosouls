#pragma once
#include <algorithm>
#include <cmath>
namespace SCContact {
struct Point {float x,y,z;Point operator+(Point b)const{return{x+b.x,y+b.y,z+b.z};}Point operator-(Point b)const{return{x-b.x,y-b.y,z-b.z};}Point operator*(float n)const{return{x*n,y*n,z*n};}};
struct Capsule {Point feet;float bottom,top,radius;};
inline bool finite(Point p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
inline bool valid(const Capsule& c){return finite(c.feet)&&std::isfinite(c.bottom)&&std::isfinite(c.top)&&std::isfinite(c.radius)&&c.radius>=.05f&&c.radius<=20&&c.bottom>=0&&c.top>=c.bottom&&c.top+c.radius<=50;}
// Nearest point IN the volume, rather than distance to the actor's origin.
// When already overlapping, keep a small direction toward the axis so guest
// facing tests remain defined. No range is enlarged by the full enemy radius.
inline Point nearest(const Capsule& c,Point from){
 Point axis={c.feet.x,std::clamp(from.y,c.feet.y+c.bottom,c.feet.y+c.top),c.feet.z};
 Point d=axis-from;float length=std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);
 if(length<.00001f)return axis;
 float travel=length>c.radius?length-c.radius:std::min(.01f,length);
 return from+d*(travel/length);
}
// Automatic downward attacks must approach the upper body, not an enemy's feet.
inline Point upper(const Capsule& c,Point from){
 float dx=from.x-c.feet.x,dz=from.z-c.feet.z,d=std::sqrt(dx*dx+dz*dz);
 float scale=d>c.radius?c.radius/d:1;
 return{c.feet.x+dx*scale,c.feet.y+c.top+std::sqrt(std::max(0.f,c.radius*c.radius-std::min(d*d,c.radius*c.radius))),c.feet.z+dz*scale};
}
inline bool inReach(Point feet,Point target,float radius,float minY,float maxY){
 Point d=target-feet;return finite(d)&&radius>=0&&d.x*d.x+d.z*d.z<=radius*radius&&d.y>=minY&&d.y<=maxY;
}
}
