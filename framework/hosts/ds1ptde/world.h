#pragma once
// Native PTDE movement submission: ESI=ChrPhysics, EDX=three vec4s.
// The game copies these vectors into ChrPhysics+60 before its Havok step.
extern "C" { void* characterOriginalMotion=nullptr; }
static bool worldMotionReady=false,worldObjectDamageReady=false;
static int worldMode=0;
static SRWLOCK worldLock=SRWLOCK_INIT;
static SCMotion::Mailbox worldMailbox;
static SCSession::Identity worldBinding{};
static DWORD worldHeartbeat=0;
static uint32_t worldSubmissions=0,worldAcknowledgements=0,worldRejected=0;
static bool worldPaused=false;
static SCMotion::Point motionPoint(Vec p){return{p.x,p.y,p.z};}
struct WorldContact{Vec point,normal;float distance;uintptr_t owner;int kind;};
static std::vector<WorldContact> worldContacts;
static void worldHostSurfaces(){
 worldContacts.clear();SC_Surface triangles[48]={};unsigned used=0;
 if(worldMode!=1||!worldMotionReady||!characterOwned()){guest->surfaces(nullptr,0,1);return;}
 uintptr_t proxy=read<uintptr_t>(posData+0x38),data=read<uintptr_t>(proxy+0x10),self=read<uintptr_t>(proxy+0x60)+0x10;
 int count=read<int>(proxy+0x14);Vec feet=read<Vec>(posData+0x10);
 if(data&&count>=0&&count<=64)for(int i=0;i<count&&used<48;i++){
  struct Raw{Vec point;float w;Vec normal;float distance;uintptr_t a;uint32_t keyA;uintptr_t b;uint32_t keyB;} raw{};
  if(!get(data+i*0x30,raw)||!isfinite(raw.distance)||raw.distance<-.5f||raw.distance>.18f)continue;
  Vec n=raw.normal,delta=raw.point-feet;float nn=dot(n,n);
  if(!isfinite(nn)||nn<.8f||nn>1.2f||!isfinite(dot(delta,delta))||dot(delta,delta)>9)continue;
  uintptr_t body=raw.a==self?raw.b:raw.b==self?raw.a:0;if(!body)continue;
  uintptr_t worldObject=body+read<int8_t>(body+0x10),userData=read<uintptr_t>(worldObject+0xc),tag=read<uintptr_t>(userData);
  int kind=read<uint16_t>(tag+4);uintptr_t owner=read<uintptr_t>(tag+8);
  worldContacts.push_back({raw.point,n,raw.distance,owner,kind});
  // A tangent patch on an actor capsule is not a walkable platform. Let native
  // capsule collision block actors; don't invent guest floors on their shoulders.
  if(kind==2)continue;
  // Only local contact patches are projected into the guest. Native Havok remains
  // authoritative; patches are refreshed every frame and never outlive contact.
  Vec u=unit(cross(n,fabsf(n.y)>.7f?Vec{0,0,1}:Vec{0,1,0})),v=cross(n,u);
  float span=fabsf(n.y)>.7f?.42f:.7f;u=u*span;v=v*(fabsf(n.y)>.7f?.42f:1.f);
  Vec corners[4]={raw.point-u-v,raw.point+u-v,raw.point+u+v,raw.point-u+v};
  int indices[6]={0,1,2,0,2,3};for(int t=0;t<2;t++)for(int k=0;k<3;k++){Vec local=(corners[indices[t*3+k]]-origin)*100;auto& dest=triangles[used+t].vertices[k];dest[0]=int(lroundf(local.x));dest[1]=int(lroundf(local.y));dest[2]=int(lroundf(local.z));}used+=2;
 }
 guest->surfaces(triangles,used,1);
}
static bool worldLineClear(Vec a,Vec b,uintptr_t target=0){
 for(const auto& c:worldContacts){if(c.owner==target||c.kind==2||fabsf(c.normal.y)>.7f)continue;
  float from=dot(a-c.point,c.normal),to=dot(b-c.point,c.normal);if(from<-.05f||to>=-.05f||from-to<.001f)continue;
  Vec at=a+(b-a)*(from/(from-to));if(dot(at-c.point,at-c.point)<1.f)return false;
 }return true;
}
extern "C" __attribute__((used,noinline)) void characterWorldMotion(uintptr_t physics,uintptr_t motion){
 if(!physics)return;
 bodyNativeMotion(physics);
 DWORD now=GetTickCount();
 AcquireSRWLockExclusive(&worldLock);
 if(worldMode!=1||physics!=worldBinding.physics||!worldBinding.valid()||chain(playerSlot,{0,4,0})!=worldBinding.actor||read<uint32_t>(worldBinding.actor+0x28)!=worldBinding.movement||DWORD(now-worldHeartbeat)>250){ReleaseSRWLockExclusive(&worldLock);return;}
 Vec current=read<Vec>(physics+0x10),delta{};SCMotion::Ticket ticket;
 if(!worldPaused&&worldMailbox.consume(now,ticket)){
  delta={ticket.to.x-current.x,ticket.to.y-current.y,ticket.to.z-current.z};
  if(!isfinite(dot(delta,delta))||dot(delta,delta)>4){delta={};++worldRejected;}else{put(physics+4,ticket.yaw);++worldSubmissions;}
 }
 // Never chase an old destination twice. Idle hooks submit neutral intent while
 // Havok continues resolving the capsule and native forces normally.
 float angle=read<float>(physics+4),cs=cosf(angle),sn=sinf(angle);float vectors[12]={cs*delta.x-sn*delta.z,delta.y,sn*delta.x+cs*delta.z,0,0,0,0,0,1,1,1,0};SIZE_T n;WriteProcessMemory(GetCurrentProcess(),(void*)motion,vectors,sizeof(vectors),&n);
 ReleaseSRWLockExclusive(&worldLock);
}
static void worldRelease(){AcquireSRWLockExclusive(&worldLock);worldMailbox.reset();worldBinding={};worldPaused=false;ReleaseSRWLockExclusive(&worldLock);worldContacts.clear();}
static void worldActivate(){AcquireSRWLockExclusive(&worldLock);worldMode=1;worldMailbox.bind();worldBinding={player,posData,lastMap,loadedMap};worldHeartbeat=GetTickCount();worldPaused=false;ReleaseSRWLockExclusive(&worldLock);}
static void worldPause(bool paused){AcquireSRWLockExclusive(&worldLock);worldHeartbeat=GetTickCount();if(paused!=worldPaused){worldMailbox.bind();worldPaused=paused;}ReleaseSRWLockExclusive(&worldLock);}
static bool worldReady(){AcquireSRWLockShared(&worldLock);bool ok=worldMailbox.ready()&&!worldPaused;ReleaseSRWLockShared(&worldLock);return ok;}
static bool worldReconcile(bool& resolvedTick){
 resolvedTick=false;
 if(worldMode!=1||!worldMotionReady)return false;
 SCMotion::Ticket ticket;AcquireSRWLockExclusive(&worldLock);worldHeartbeat=GetTickCount();bool expired=worldMailbox.expired(worldHeartbeat),resolved=worldMailbox.acknowledge(ticket),busy=worldMailbox.busy();uint32_t generation=worldMailbox.generation(),submissions=worldSubmissions,rejected=worldRejected;ReleaseSRWLockExclusive(&worldLock);
 if(expired){logf("WORLD motion timeout; native handback");return false;}if(busy)return true;
 Vec host=read<Vec>(posData+0x10),current=origin+Vec{state.position[0],state.position[1],state.position[2]}*.01f,correction=host-current;
 if(!isfinite(dot(correction,correction))||dot(correction,correction)>4)return false;
 SC_MotionResolution r={};r.size=sizeof(r);r.generation=generation;r.sequence=resolved?ticket.sequence:0;Vec local=(host-origin)*100;r.position[0]=local.x;r.position[1]=local.y;r.position[2]=local.z;
 if(resolved){
  ++worldAcknowledgements;Vec wanted={ticket.to.x-ticket.from.x,ticket.to.y-ticket.from.y,ticket.to.z-ticket.from.z};
  // Only planes from actual native contacts may clip guest momentum. Positional
  // disagreement itself is not evidence of a wall, ceiling or ground.
  for(const auto&c:worldContacts){if(r.normalCount>=4)break;Vec n=c.normal;float into=dot(wanted,n);if(into<-.001f&&dot(correction,n)>.005f){auto index=r.normalCount++;r.normals[index][0]=n.x;r.normals[index][1]=n.y;r.normals[index][2]=n.z;}}
 }
 if(!guest->resolveMotion(character,&r,&state,&geo)||!SC::frame(state,geo,*profile))return false;
 resolvedTick=resolved;
 static DWORD lastLog=0;if(GetTickCount()-lastLog>1000){lastLog=GetTickCount();logf("WORLD bridge submitted=%u ack=%u rejected=%u error_cm=%.3f planes=%u generation=%u",submissions,worldAcknowledgements,rejected,sqrtf(dot(correction,correction))*100,r.normalCount,r.generation);}
 return true;
}
static bool worldSubmit(Vec from,Vec destination){AcquireSRWLockExclusive(&worldLock);bool ok=worldMailbox.publish(motionPoint(from),motionPoint(destination),state.faceAngle+3.14159265f,GetTickCount());ReleaseSRWLockExclusive(&worldLock);return ok;}
__attribute__((naked)) static void characterMotionHook(){__asm__ volatile(
 "pushfl\n pushal\n subl $128,%esp\n"
 "movups %xmm0,0(%esp)\n movups %xmm1,16(%esp)\n movups %xmm2,32(%esp)\n movups %xmm3,48(%esp)\n movups %xmm4,64(%esp)\n movups %xmm5,80(%esp)\n movups %xmm6,96(%esp)\n movups %xmm7,112(%esp)\n"
 "pushl %edx\n pushl %esi\n call _characterWorldMotion\n addl $8,%esp\n"
 "movups 0(%esp),%xmm0\n movups 16(%esp),%xmm1\n movups 32(%esp),%xmm2\n movups 48(%esp),%xmm3\n movups 64(%esp),%xmm4\n movups 80(%esp),%xmm5\n movups 96(%esp),%xmm6\n movups 112(%esp),%xmm7\n"
 "addl $128,%esp\n popal\n popfl\n jmp *_characterOriginalMotion\n");}
static bool worldInstall(){
 unsigned char bytes[6];const unsigned char expected[]={0xff,0x86,0xe4,0,0,0};
 if(!get(uintptr_t(0xebdc40),bytes)||memcmp(bytes,expected,6))return false;
 if(MH_CreateHook((void*)0xebdc40,(void*)characterMotionHook,&characterOriginalMotion)!=MH_OK)return false;
 worldMotionReady=MH_EnableHook((void*)0xebdc40)==MH_OK;
 const unsigned char damageExpected[]={0x51,0xd9,0x05,0xa4,0xfb,0x15,0x01,0x53,0x56,0x8b,0x74,0x24,0x10};unsigned char damageActual[sizeof(damageExpected)]={};worldObjectDamageReady=get(uintptr_t(0xdea8e0),damageActual)&&!memcmp(damageActual,damageExpected,sizeof(damageExpected));
 logf("Native world motion hook=%d object entry=%d",worldMotionReady,worldObjectDamageReady);return worldMotionReady;
}
