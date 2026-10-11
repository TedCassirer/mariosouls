#pragma once
#include "contact.h"
#include <unordered_set>
#include <cstdint>
namespace SCEquipment {
// Presentation/action timing is shared. Native hosts supply the item's actual
// stamina cost and damage rules. Equipment changes invalidate a pending attack.
class Swing {
 bool held=false,queued=false;int rest=0,nextCombo=0;
public:
 int age=-1,combo=0,weapon=-1,serial=0;std::unordered_set<int> targets;
 void cancel(){age=-1;weapon=-1;queued=false;rest=0;nextCombo=0;targets.clear();}
 void reset(){cancel();held=false;}
 bool running()const{return age>=0;}
 bool active()const{return age>=5&&age<=12;}
 float phase()const{return running()?std::min(1.f,age/23.f):0;}
 // Returns an intent; caller must confirm affordability BEFORE begin(). Presses
 // in late recovery queue one attack, never an unlimited held-button chain.
 bool tick(bool pressed,bool eligible,int equipped){
  bool edge=pressed&&!held;held=pressed;
  if(!eligible||(running()&&weapon!=equipped)){cancel();return false;}
  if(running()){
   if(edge&&age>=12)queued=true;
   if(++age<24)return false;
   age=-1;rest=12;nextCombo=1-combo;
   if(queued){queued=false;return true;}
  }
  if(rest>0)--rest;else nextCombo=0;
  return edge;
 }
 int intendedCombo()const{return nextCombo;}
 void begin(int equipped){weapon=equipped;combo=nextCombo;age=0;++serial;targets.clear();}
 bool claim(int target){return active()&&targets.size()<16&&targets.insert(target).second;}
};
using Point=SCContact::Point;
inline float dot(Point a,Point b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Point segmentPoint(Point a,Point b,Point p){Point v=b-a;float n=dot(v,v);return a+v*(n>1e-8f?std::clamp(dot(p-a,v)/n,0.f,1.f):0);}
// Closest sword/capsule axis pair; analytic segment-segment distance (including
// degenerate segments). Contact is the surface toward the blade, not NPC feet.
inline bool contact(Point a,Point b,const SCContact::Capsule& c,float radius,Point& hit){
 if(!SCContact::finite(a)||!SCContact::finite(b)||!SCContact::valid(c)||!std::isfinite(radius)||radius<0||radius>.5f)return false;
 Point q=c.feet+Point{0,c.bottom,0},q2=c.feet+Point{0,c.top,0};
 Point d1=b-a,d2=q2-q,r=a-q;float aa=dot(d1,d1),ee=dot(d2,d2),ff=dot(d2,r),s=0,t=0;
 if(aa<=1e-8f){t=ee>1e-8f?std::clamp(ff/ee,0.f,1.f):0;}
 else {float cc=dot(d1,r);if(ee<=1e-8f)s=std::clamp(-cc/aa,0.f,1.f);else{
  float bb=dot(d1,d2),denom=aa*ee-bb*bb;if(denom>1e-8f)s=std::clamp((bb*ff-cc*ee)/denom,0.f,1.f);
  t=(bb*s+ff)/ee;if(t<0){t=0;s=std::clamp(-cc/aa,0.f,1.f);}else if(t>1){t=1;s=std::clamp((bb-cc)/aa,0.f,1.f);}
 }}
 Point blade=a+d1*s,axis=q+d2*t,d=blade-axis;float n=dot(d,d);
 if(n>(c.radius+radius)*(c.radius+radius))return false;
 hit=n>1e-8f?axis+d*(std::min(c.radius,std::sqrt(n))/std::sqrt(n)):blade;return true;
}
inline bool sweep(Point prevA,Point prevB,Point a,Point b,const SCContact::Capsule& c,float radius,Point& hit){
 if(!SCContact::finite(prevA)||!SCContact::finite(prevB))return false;
 if(dot(a-prevA,a-prevA)>4||dot(b-prevB,b-prevB)>9)return false; // Warp/stale socket.
 // Bounded interpolation covers the swept blade between accepted 30 Hz frames.
 for(int i=0;i<=8;i++){float t=i/8.f;if(contact(prevA+(a-prevA)*t,prevB+(b-prevB)*t,c,radius,hit))return true;}return false;
}
}
