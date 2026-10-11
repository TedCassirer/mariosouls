#pragma once
#include "../sdk/class_api.h"
#include <cmath>
#include <cstring>
namespace SCClass {
inline bool valid(const SC_ClassProfile* p){
 return p&&p->size==sizeof(*p)&&p->archetype==SC_CLASS_FIRECASTER&&p->name&&p->description&&
  p->name[0]&&p->description[0]&&std::strlen(p->name)<=31&&std::strlen(p->description)<=191&&
  std::isfinite(p->previewHeightCm)&&p->previewHeightCm>=20&&p->previewHeightCm<=500&&
  std::isfinite(p->previewYaw)&&std::fabs(p->previewYaw)<=6.284f;
}
// One automatic decision per loaded actor. A manual toggle stays authoritative
// for that actor; respawn/reload receives the native class default again.
struct Selection {
 uintptr_t actor=0;
 bool observe(uintptr_t current,bool matches){
  if(!current){actor=0;return false;}
  if(current==actor)return false;
  actor=current;return matches;
 }
};
}
