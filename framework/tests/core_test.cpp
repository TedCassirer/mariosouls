#undef NDEBUG
#include "../core/combat.h"
#include "../core/validation.h"
#include "../core/camera.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main(){
 SC_CameraProfile camera={sizeof(camera),SC_CAMERA_PROFILE_VERSION,1.5f};assert(SCCamera::valid(&camera));
 camera.distanceScale=NAN;assert(!SCCamera::valid(&camera));camera.distanceScale=3;assert(!SCCamera::valid(&camera));
 SCCamera::Distance distance(4);float target=0;assert(distance.next(4,1.5f,target)&&target==6);distance.written(target);
 for(int i=0;i<300;i++){assert(distance.next(6,1.5f,target)&&target==6);distance.written(target);}
 assert(distance.next(6,1.25f,target)&&target==5);distance.written(target);assert(distance.restore(5,target)&&target==4);
 assert(!distance.next(7,1.5f,target));assert(!distance.restore(7,target));
 puts("PASS camera: profile bounds, no accumulated zoom, configuration change, exact restore, external edit conflict");
 SC_Descriptor d{};d.size=sizeof(d);d.abiVersion=SC_ABI_VERSION;strcpy(d.id,"test_actor");strcpy(d.displayName,"Synthetic test actor");d.maxTriangles=12;d.textureWidth=d.textureHeight=1;d.attackCount=3;d.automaticAttackId=2;d.basePower=100;d.upgradePower=10;d.dexterityWeight=.5f;
 for(int i=0;i<3;i++){auto&a=d.attacks[i];sprintf(a.name,"move%d",i);a.multiplier=1;a.staminaCost=8;a.activeTicks=10;a.maxTargets=2;a.rangeCm=100;a.minHeightCm=-100;a.maxHeightCm=100;}
 d.attacks[2].flags=SC_ATTACK_AUTOMATIC;d.attacks[2].automaticRadiusCm=50;d.attacks[2].automaticMinHeightCm=-170;d.attacks[2].automaticMaxHeightCm=-35;
 d.body={1,0,0,0,0,1,1,0};
 assert(SC::descriptor(&d));auto bad=d;bad.abiVersion=99;assert(!SC::descriptor(&bad));bad=d;bad.attackCount=33;assert(!SC::descriptor(&bad));bad=d;bad.textureWidth=10000;assert(!SC::descriptor(&bad));bad=d;bad.attacks[0].multiplier=std::numeric_limits<float>::quiet_NaN();assert(!SC::descriptor(&bad));
 bad=d;bad.attacks[2].automaticRadiusCm=std::numeric_limits<float>::infinity();assert(!SC::descriptor(&bad));bad=d;bad.attacks[0].minHeightCm=101;assert(!SC::descriptor(&bad));bad=d;bad.capabilities=0x80000000;assert(!SC::descriptor(&bad));
 SCCombat::Window w;SC_State s{};s.attackId=0;s.interruptedAttackId=-1;s.flags=SC_ATTACK_ACTIVE|SC_GROUNDED;
 assert(w.observe(0,true,2));assert(w.pay(20,8,65));assert(!w.pay(20,8,65));assert(w.active(s,d));assert(w.canHit(100,d));w.hit.insert(100);assert(!w.canHit(100,d));w.hit.insert(101);assert(!w.canHit(102,d));
 assert(!w.observe(0,true,2));assert(w.observe(1,true,2));assert(w.pay(20,8,65));assert(w.canHit(100,d));s.attackId=1;assert(w.active(s,d));
 for(int i=0;i<11;i++)w.observe(1,true,2);assert(!w.active(s,d));
 w.reset();assert(w.observe(0,true,2));assert(!w.pay(7,8,65));assert(w.exhausted);assert(!w.active(s,d));assert(!w.pay(100,8,65));
 w.reset();assert(w.beginAutomatic(2));assert(!w.pay(0,16,90));assert(!w.beginAutomatic(2));w.observe(-1,false,2);assert(!w.beginAutomatic(2));w.observe(-1,true,2);assert(w.beginAutomatic(2));assert(w.pay(20,16,90));s.velocity[1]=-50;assert(w.active(s,d));s.velocity[1]=50;assert(!w.active(s,d));
 SCCombat::Settings balance;assert(SCCombat::power(balance,10,10)==100);assert(SCCombat::power(balance,40,40)>100);balance.fixedPower=75;assert(SCCombat::power(balance,99,99)==75);
 assert(SCCombat::fallDamage(5,500)==0);assert(SCCombat::fallDamage(20,500)==500);
 SC_Mesh mesh{};mesh.capacityTriangles=12;s={};s.attackId=s.interruptedAttackId=-1;assert(SC::frame(s,mesh,d));s.position[0]=std::numeric_limits<float>::infinity();assert(!SC::frame(s,mesh,d));s.position[0]=0;mesh.numTrianglesUsed=13;assert(!SC::frame(s,mesh,d));
 SC_Body body={.9f,22,105,24,110,.05f,.05f,1};assert(SCBody::valid(body));
 assert(SCBody::landingDamage(594,594,594,body)==29);assert(SCBody::landingDamage(594,1,594,body)==0);assert(SCBody::landingDamage(99,50,594,body)==5);
 auto cap=SCBody::capsule(22,105);assert(fabsf(cap.bottom-cap.radius)<.00001f);assert(fabsf(cap.top+cap.radius-1.05f)<.00001f);
 assert(SCBody::modelCoordinate(200,100,.9f)==190);body.heightCm=20;assert(!SCBody::valid(body));body.heightCm=105;body.modelScale=NAN;assert(!SCBody::valid(body));
 puts("PASS shared policy: ABI/bounds, distinct attacks, exhaustion, per-target limits, automatic-attack airtime, scaling, hazards, invalid frames");
}
