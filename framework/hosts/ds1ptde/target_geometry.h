#pragma once
#include "../../core/contact.h"
// Same native hkpCapsuleShape layout verified for the player, read-only for NPCs.
// A body point is an attack candidate; native defenses/part rules still decide damage.
static bool targetShapeCapsule(uintptr_t shape,Vec feet,SCContact::Capsule& out){
 if(!shape||!bodyShapeValid(shape))return false;
 float points[8]={},radius=0;
 if(!get(shape+0x20,points)||!get(shape+0x10,radius))return false;
 for(float v:points)if(!isfinite(v))return false;
 if(fabsf(points[0])+fabsf(points[2])+fabsf(points[4])+fabsf(points[6])>.001f)return false;
 out={{feet.x,feet.y,feet.z},points[5],points[1],radius};return SCContact::valid(out);
}
static bool targetCapsule(uintptr_t npc,Vec feet,SCContact::Capsule& out){
 uintptr_t physics=chain(npc+0x28,{0,0x1c});
 return physics&&targetShapeCapsule(read<uintptr_t>(physics+0x3c),feet,out);
}
static Vec targetContact(const SCContact::Capsule& capsule,Vec feet,bool automatic){
 auto point=automatic?SCContact::upper(capsule,{feet.x,feet.y,feet.z}):SCContact::nearest(capsule,{feet.x,feet.y+.7f,feet.z});
 return{point.x,point.y,point.z};
}
