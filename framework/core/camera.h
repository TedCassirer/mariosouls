#pragma once
#include <cmath>
#include "../sdk/character_api.h"
namespace SCCamera {
inline bool validScale(float scale){return std::isfinite(scale)&&scale>=1.f&&scale<=2.f;}
inline bool valid(const SC_CameraProfile*p){return p&&p->size==sizeof(*p)&&p->version==SC_CAMERA_PROFILE_VERSION&&validScale(p->distanceScale);}
// Own only the value we actually wrote. An external edit revokes this lease.
struct Distance {
 float original=0,last=0;bool conflict=false;
 explicit Distance(float value):original(value),last(value){}
 bool next(float current,float scale,float&value){
  if(conflict||current!=last||!validScale(scale)){conflict=true;return false;}
  value=original*scale;return std::isfinite(value)&&value>=.5f&&value<=40.f;
 }
 void written(float value){last=value;}
 bool restore(float current,float&value){
  if(conflict||current!=last)return false;
  value=original;return true;
 }
};
}
