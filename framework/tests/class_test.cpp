#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <limits>
#include "../core/starting_class.h"
int main(){
 SC_ClassProfile p={sizeof(p),SC_CLASS_FIRECASTER,"Guest","Guest class",160,0};
 assert(SCClass::valid(&p));assert(!SCClass::valid(nullptr));
 p.size=0;assert(!SCClass::valid(&p));p.size=sizeof(p);
 p.previewHeightCm=0;assert(!SCClass::valid(&p));p.previewHeightCm=160;
 p.previewYaw=std::numeric_limits<float>::quiet_NaN();assert(!SCClass::valid(&p));p.previewYaw=0;
 p.archetype=99;assert(!SCClass::valid(&p));p.archetype=SC_CLASS_FIRECASTER;
 SCClass::Selection s;using D=SCClass::Decision;
 assert(s.observe(0,false,false,-1,false)==D::None);
 assert(s.observe(100,true,true,1801,true)==D::Enable);
 assert(s.observe(100,true,true,1801,true)==D::Enable); // persistent, not one-shot
 assert(s.observe(100,false,false,-1,true)==D::Enable); // class unreadable while streaming
 s.manual(100);assert(!s.required());
 assert(s.observe(100,true,true,1801,true)==D::None); // deliberate off remains for this life
 assert(s.observe(100,true,true,1801,false)==D::None);
 assert(s.observe(100,true,true,1801,true)==D::Enable); // same-pointer respawn resets override
 s.manual(100);assert(s.observe(100,true,true,1002,true)==D::Enable); // same-pointer zone change
 assert(s.observe(0,false,false,-1,false)==D::Disable);
 assert(s.observe(100,false,false,1801,true)==D::None); // recycled actor waits for class
 assert(s.observe(100,true,true,1801,true)==D::Enable);
 assert(s.observe(200,true,false,1801,true)==D::Disable); // another save must not inherit Mario
 assert(s.observe(200,true,false,1801,true)==D::None);
 assert(s.observe(200,true,true,1801,true)==D::Enable); // saved stats arriving late
 assert(s.observe(200,true,false,1801,true)==D::Disable); // same-pointer different class
 puts("PASS starting class: persistent identity, transient reads, same-pointer respawn/zone, manual override and save isolation");
}
