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
 SCClass::Selection s;
 assert(!s.observe(0,true));assert(s.observe(100,true));
 assert(!s.observe(100,true)); // manual off must not be overwritten next frame
 assert(!s.observe(0,true));assert(s.observe(100,true)); // recycled pointer after load
 assert(!s.observe(200,false));assert(!s.observe(200,true));
 assert(s.observe(300,true));
 puts("PASS starting class: profile bounds, per-actor activation, manual override, native classes and recycled identity");
}
