#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../../sdk/character_api.h"
#include "../../sdk/launcher_api.h"
#include "../../core/motion.h"
#include "../../../work/libsm64/src/libsm64.h"
#include "profile.generated.h"

// libsm64 and ROM formats are private to this adapter. The host never sees them.
static HMODULE runtime=nullptr;
static bool initialized=false;
static SC_Log report=nullptr;
static std::vector<uint8_t> rom;
static SM64MarioState raw{};
static uint32_t previousAction=0;
static int32_t active=-1;
static uint32_t (*libCannonBegin)(int32_t,const float*,float);
static void (*libCannonCancel)(int32_t);
static uint32_t (*libCannonQuery)(int32_t,float*);
static bool cannonAssetsReady=false;
#include "cannon_assets.h"
static decltype(&sm64_global_init) libInit;
static decltype(&sm64_global_terminate) libShutdown;
static decltype(&sm64_audio_init) libAudioInit;
static decltype(&sm64_audio_tick) libAudio;
static decltype(&sm64_static_surfaces_load) libSurfaces;
static decltype(&sm64_host_contacts_load) libContacts;
static decltype(&sm64_mario_create) libCreate;
static decltype(&sm64_mario_delete) libDestroy;
static decltype(&sm64_mario_tick) libTick;
static decltype(&sm64_mario_host_pose_tick) libPose;
static decltype(&sm64_mario_host_resume) libResume;
static decltype(&sm64_mario_hand_matrix) libHand;
static uint32_t (*libEquipment)(int32_t,uint32_t,uint32_t,uint32_t,float,float);
static uint32_t (*libEquipmentHands)(int32_t,float*,float*);
static SC_Attachments equipmentHands={sizeof(SC_Attachments)};
static SC_EquipmentPose equipmentPose={sizeof(SC_EquipmentPose)};
static bool fireAppearance=false;
static void firePresentation(SC_Mesh&,bool);
static decltype(&sm64_set_mario_position) libPosition;
static decltype(&sm64_set_mario_velocity) libVelocity;
static decltype(&sm64_set_mario_forward_velocity) libForward;
static decltype(&sm64_set_mario_action) libAction;
static decltype(&sm64_set_mario_faceangle) libFace;
static void (*libBody)(float,float);
static void (*libHealth)(int32_t,int32_t);
static decltype(&sm64_surface_find_floor_height) libFloor;
static uint32_t (*libProbe)(int32_t,float,float,float);
static decltype(&sm64_mario_attack) libContact;

enum Move { None=-1,Punch1,Punch2,Kick,JumpKick,Dive,SlideKick,Stomp,Pound };
static int identify(uint32_t a,unsigned arg){
    if(a==0x00800380||a==0x00800457){if(arg<=2)return Punch1;if(arg<=5)return Punch2;if(arg==6)return Kick;if(arg==9)return SlideKick;return None;}
    if(a==0x018008ac)return JumpKick;
    if(a==0x0188088a||a==0x00880456)return Dive;
    if(a==0x018008aa||a==0x0080045a)return SlideKick;
    if(a==0x008008a9||a==0x0080023c)return Pound;
    return None;
}
static bool equipmentActionEligible(uint32_t action){
    // Explicit ordinary locomotion allowlist. A non-air action alone is not a
    // safe indication: damage, burning, scripted states and recovery also land
    // on the floor. Keep that guest taxonomy inside its adapter.
    switch(action){
        case 0x0c400201:case 0x0c400205:case 0x0c400209: // idle, panting, wall
        case 0x04000440:case 0x00000443:case 0x00000444: // walk and turn
        case 0x04000445:case 0x0400044a:case 0x0c00023d: // brake/decelerate
        case 0x0c000230:case 0x0c000231:case 0x0c000232:case 0x0c000233:
        case 0x04000470:case 0x04000471:case 0x04000472:case 0x04000473:
            return true;
        default:return false;
    }
}
static void state(SC_State*out){
    *out={};for(int i=0;i<3;i++){out->position[i]=raw.position[i];out->velocity[i]=raw.velocity[i]*SC_FIXED_HZ;}
    out->faceAngle=raw.faceAngle;out->forwardSpeed=raw.forwardVelocity*SC_FIXED_HZ;out->floorHeight=raw.floorHeight;
    out->rawAction=raw.action;out->rawFlags=raw.flags;out->rawActionArg=raw.actionArg;out->rawActionTimer=raw.actionTimer;out->rawAnimation=raw.animID;out->rawAnimationFrame=raw.animFrame;
    out->attackId=identify(raw.action,raw.actionArg);out->interruptedAttackId=identify(raw.interruptedAction,0);
    if(!(raw.action&0x800)){out->flags|=SC_CAN_INTERACT;if(raw.position[1]<=raw.floorHeight+5)out->flags|=SC_GROUNDED;}
    if((out->flags&SC_GROUNDED)&&equipmentActionEligible(raw.action))out->flags|=SC_CAN_EQUIP_ACTION;
    if(!(raw.action&0x20000))out->flags|=SC_AUTO_ATTACK_ELIGIBLE;
    if(raw.action==0x00001371||raw.action==0x00880898)out->flags=0;
    bool hitting=false;
    switch(out->attackId){
        case Punch1:case Punch2:hitting=raw.flags&0x00100000;break;
        case Kick:case JumpKick:hitting=raw.flags&0x00200000;break;
        case Dive:case SlideKick:hitting=true;break;
        case Pound:hitting=(raw.action==0x008008a9&&raw.velocity[1]<0)||(raw.action==0x0080023c&&raw.actionTimer<=2);break;
    }
    if(hitting)out->flags|=SC_ATTACK_ACTIVE;
    if(raw.action==0x0080023c&&previousAction!=raw.action)out->landingRecoveryTicks=12;
    previousAction=raw.action;
}
static bool geometry(SC_Mesh*m,SM64MarioGeometryBuffers&g){
    if(!m||m->capacityTriangles<SM64_GEO_MAX_TRIANGLES||!m->position||!m->normal||!m->color||!m->uv)return false;
    g={m->position,m->normal,m->color,m->uv,0};return true;
}
static const SC_Descriptor* describe(){return &characterProfile;}
static void clearEquipment(int32_t id){
    if(initialized&&id>=0&&id==active&&libEquipment)libEquipment(id,0,0,0,0,0);
    equipmentHands={sizeof(SC_Attachments)};equipmentPose={sizeof(SC_EquipmentPose)};
}
static void captureEquipment(int32_t id){
    equipmentHands={sizeof(SC_Attachments)};
    equipmentHands.valid=libEquipmentHands(id,equipmentHands.right,equipmentHands.left);
    // Native shield bind geometry has its pointed bottom along local +X.
    // Roll that asset frame onto the glove without changing its outward normal.
    if((equipmentHands.valid&2)&&equipmentPose.leftKind==SC_EQUIPMENT_SHIELD)for(int row=0;row<3;row++){
        float x=equipmentHands.left[row];equipmentHands.left[row]=-equipmentHands.left[4+row];equipmentHands.left[4+row]=x;
    }
}
static void shiftEquipment(float x,float y,float z){
    float delta[3]={x,y,z};
    for(int axis=0;axis<3;axis++){
        if(equipmentHands.valid&1)equipmentHands.right[12+axis]+=delta[axis];
        if(equipmentHands.valid&2)equipmentHands.left[12+axis]+=delta[axis];
    }
}
static void shutdown(){
    clearEquipment(active);
    if(initialized){if(active>=0)libDestroy(active);libShutdown();}
    initialized=false;active=-1;fireAppearance=false;raw={};rom.clear();if(runtime)FreeLibrary(runtime);runtime=nullptr;
}
static uint32_t initialize(const SC_Init* init,SC_Texture*texture){
    if(initialized||!init||init->size!=sizeof(*init)||!texture||!texture->rgba||texture->capacityBytes<SM64_TEXTURE_WIDTH*SM64_TEXTURE_HEIGHT*4)return 0;
    report=init->log;
    runtime=LoadLibraryExA(init->runtimePath,nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
    if(!runtime){if(report)report("Mario runtime could not be loaded");return 0;}
#define LOAD(var,name) var=(decltype(var))GetProcAddress(runtime,#name);if(!var){if(report)report("Missing " #name);shutdown();return 0;}
    LOAD(libInit,sm64_global_init);LOAD(libShutdown,sm64_global_terminate);LOAD(libAudioInit,sm64_audio_init);LOAD(libAudio,sm64_audio_tick);
    LOAD(libVelocity,sm64_set_mario_velocity);LOAD(libForward,sm64_set_mario_forward_velocity);
    LOAD(libSurfaces,sm64_static_surfaces_load);LOAD(libContacts,sm64_host_contacts_load);LOAD(libCreate,sm64_mario_create);LOAD(libDestroy,sm64_mario_delete);
    LOAD(libTick,sm64_mario_tick);LOAD(libPose,sm64_mario_host_pose_tick);LOAD(libResume,sm64_mario_host_resume);LOAD(libHand,sm64_mario_hand_matrix);
    LOAD(libEquipment,sm64_mario_equipment);LOAD(libEquipmentHands,sm64_mario_equipment_hands);
    LOAD(libPosition,sm64_set_mario_position);LOAD(libAction,sm64_set_mario_action);LOAD(libFace,sm64_set_mario_faceangle);LOAD(libHealth,sm64_mario_host_health);
    LOAD(libFloor,sm64_surface_find_floor_height);LOAD(libProbe,sm64_mario_probe_attack);LOAD(libContact,sm64_mario_attack);
    LOAD(libBody,sm64_host_body);
    LOAD(libCannonBegin,sm64_mario_cannon_begin);LOAD(libCannonCancel,sm64_mario_cannon_cancel);LOAD(libCannonQuery,sm64_mario_cannon_query);
#undef LOAD
    FILE*f=init->assetPath?fopen(init->assetPath,"rb"):nullptr;
    if(!f){if(report)report("Owned US Mario 64 ROM is missing");shutdown();return 0;}
    fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);bool ok=n==8388608;
    if(ok){rom.resize(n);ok=fread(rom.data(),1,n,f)==size_t(n);}fclose(f);
    if(!ok||rom[0]!=0x80||rom[1]!=0x37||rom[2]!=0x12||rom[3]!=0x40){if(report)report("Expected a big-endian US Mario 64 ROM");shutdown();return 0;}
    cannonAssetsReady=MarioCannonAssets::initialize(rom);
    libInit(rom.data(),texture->rgba);libAudioInit(rom.data());libBody(characterProfile.body.radiusCm,characterProfile.body.heightCm);initialized=true;return 1;
}
static int32_t create(float x,float y,float z,float yaw){
    if(!initialized||active>=0)return -1;
    active=libCreate(x,y,z);if(active>=0){fireAppearance=false;raw={};previousAction=0;clearEquipment(active);libHealth(active,0);libAction(active,0x0c400201);libFace(active,yaw);}return active;
}
static void destroy(int32_t id){if(id>=0&&id==active){clearEquipment(id);libDestroy(id);active=-1;}}
static uint32_t step(int32_t id,const SC_Input*in,SC_State*out,SC_Mesh*mesh){
    if(id!=active||id<0||!in||!out)return 0;SM64MarioGeometryBuffers g;
    if(!geometry(mesh,g))return 0;
    SM64MarioInputs input={in->camLookX,in->camLookZ,-in->moveX,-in->moveY,in->jump,in->attack,in->crouch};
    libTick(id,&input,&raw,&g);mesh->numTrianglesUsed=g.numTrianglesUsed;state(out);captureEquipment(id);firePresentation(*mesh,true);if(raw.action==0x00001371)mesh->numTrianglesUsed=0;return 1;
}

struct Vec{float x,y,z;Vec operator+(Vec b)const{return{x+b.x,y+b.y,z+b.z};}Vec operator-(Vec b)const{return{x-b.x,y-b.y,z-b.z};}Vec operator*(float s)const{return{x*s,y*s,z*s};}};
static Vec cross(Vec a,Vec b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static Vec unit(Vec v){float n=sqrtf(v.x*v.x+v.y*v.y+v.z*v.z);return n>.0001f?v*(1/n):Vec{0,0,1};}
// Original procedural accessory, fitted to this character's hand. The shared host
// only knows that this adapter supplies its own item prop.
static void flask(SC_Mesh&mesh){
    float m[16];libHand(m);Vec axis=unit({.09665986f,-.21849427f,-.9710392f});
    Vec u=unit(cross(axis,{1,0,0})),v=cross(axis,u),grip={40,0,0};
    auto transform=[&](Vec p){return Vec{p.x*m[0]+p.y*m[4]+p.z*m[8]+m[12],p.x*m[1]+p.y*m[5]+p.z*m[9]+m[13],p.x*m[2]+p.y*m[6]+p.z*m[10]+m[14]};};
    const float heights[]={-125,-110,-50,-15,0,70,75},radii[]={0,45,42,30,15,15,0};
    for(int band=0;band<6;band++)for(int side=0;side<10;side++){
        float a=side*6.2831853f/10,b=(side+1)*6.2831853f/10;Vec ra=u*cosf(a)+v*sinf(a),rb=u*cosf(b)+v*sinf(b);
        Vec q[4]={transform(grip+axis*heights[band]+ra*radii[band]),transform(grip+axis*heights[band]+rb*radii[band]),transform(grip+axis*heights[band+1]+rb*radii[band+1]),transform(grip+axis*heights[band+1]+ra*radii[band+1])};
        for(int half=0;half<2;half++){
            if(mesh.numTrianglesUsed>=mesh.capacityTriangles)return;
            int ids[3]={0,half?2:1,half?3:2};Vec n=unit(cross(q[ids[1]]-q[0],q[ids[2]]-q[0]));
            for(int k=0;k<3;k++){size_t i=mesh.numTrianglesUsed*3+k;Vec p=q[ids[k]];mesh.position[i*3]=p.x;mesh.position[i*3+1]=p.y;mesh.position[i*3+2]=p.z;mesh.normal[i*3]=n.x;mesh.normal[i*3+1]=n.y;mesh.normal[i*3+2]=n.z;mesh.color[i*3]=1;mesh.color[i*3+1]=band>=4?.75f:.4f;mesh.color[i*3+2]=.04f;mesh.uv[i*2]=mesh.uv[i*2+1]=1;}
            ++mesh.numTrianglesUsed;
        }
    }
}
#include "fire_flower.h"
static uint32_t pose(int32_t id,const SC_Pose*p,SC_State*out,SC_Mesh*mesh){
    if(id!=active||id<0||!p||!out)return 0;SM64MarioGeometryBuffers g;if(!geometry(mesh,g))return 0;
    libCannonCancel(id);
    if(p->kind!=SC_POSE_NATIVE_COMBAT)clearEquipment(id);
    int anim=0xc3,style=0;float phase=-1;
    switch(p->kind){
        case SC_POSE_CLIMB:anim=5;style=2;phase=p->travelCm*.9f;break;
        case SC_POSE_DRINK:anim=0x30;style=4;phase=p->phase;break;
        case SC_POSE_REST:anim=0x84;style=1;phase=p->phase;break;
        case SC_POSE_DOOR:anim=0x60;style=1;phase=p->phase;break;
        case SC_POSE_WALK:anim=0x48;break;
        case SC_POSE_DOWNED:anim=0x19;break;
        case SC_POSE_STAGGER:anim=0x74;break;
        case SC_POSE_DEATH:anim=0x32;style=8;break;
    }
    libPose(id,p->position[0],p->position[1],p->position[2],p->yaw,anim,phase,style,&raw,&g);
    mesh->numTrianglesUsed=g.numTrianglesUsed;state(out);
    if(p->kind==SC_POSE_NATIVE_COMBAT)captureEquipment(id);
    if(p->kind==SC_POSE_DEATH||p->kind==SC_POSE_NATIVE_COMBAT){out->flags=0;out->attackId=out->interruptedAttackId=-1;out->landingRecoveryTicks=0;}
    firePresentation(*mesh,p->kind==SC_POSE_NATIVE_COMBAT);
    if(p->kind==SC_POSE_DRINK&&p->phase>.02f)flask(*mesh);
    return 1;
}
static void resume(int32_t id,float x,float y,float z,float yaw){if(id==active&&id>=0){libCannonCancel(id);clearEquipment(id);libResume(id,x,y,z,yaw);raw.position[0]=x;raw.position[1]=y;raw.position[2]=z;raw.velocity[0]=raw.velocity[1]=raw.velocity[2]=raw.forwardVelocity=0;raw.faceAngle=yaw;previousAction=0;}}
static void position(int32_t id,float x,float y,float z){if(id==active&&id>=0){libPosition(id,x,y,z);shiftEquipment(x-raw.position[0],y-raw.position[1],z-raw.position[2]);raw.position[0]=x;raw.position[1]=y;raw.position[2]=z;}}
static void command(int32_t id,uint32_t c){if(id==active&&id>=0)libAction(id,c==SC_COMMAND_FALL?0x0100088c:0x0c400201);}
static void damage(int32_t id,uint32_t severity){if(id==active&&id>=0){if(severity){libCannonCancel(id);clearEquipment(id);}libHealth(id,std::min(severity,2u));}}
static void surfaces(const SC_Surface*s,uint32_t n,uint32_t dynamic){
    if(!initialized||(!s&&n)||n>2000000||dynamic&&n>48)return;
    std::vector<SM64Surface> dest(n);for(uint32_t i=0;i<n;i++)memcpy(dest[i].vertices,s[i].vertices,sizeof(s[i].vertices));
    if(dynamic)libContacts(dest.data(),n);else libSurfaces(dest.data(),n);
}
static float floorHeight(float x,float y,float z){return initialized?libFloor(x,y,z):-11000;}
static uint32_t probe(int32_t id,float x,float y,float z){if(id!=active||id<0)return 0;auto p=libProbe(id,x,y,z);return (p&0x3f?SC_CONTACT_ATTACK:0)|(p&0x40?SC_CONTACT_AUTOMATIC:0);}
static void contact(int32_t id,float x,float y,float z,float radius){if(id==active&&id>=0)libContact(id,x,y,z,radius);}
static uint32_t audio(uint32_t queued,uint32_t desired,int16_t*stereo,uint32_t capacity){
    if(!initialized||!stereo||capacity<1088)return 0;
    return 2*libAudio(queued,desired,stereo);
}
static uint32_t resolveMotion(int32_t id,const SC_MotionResolution*r,SC_State*out,SC_Mesh*m){
    if(id!=active||id<0||!r||r->size!=sizeof(*r)||r->normalCount>4||!out||!m||m->numTrianglesUsed>m->capacityTriangles)return 0;
    SCMotion::Point target={r->position[0],r->position[1],r->position[2]};if(!SCMotion::finite(target))return 0;
    SCMotion::Point velocity={raw.velocity[0],raw.velocity[1],raw.velocity[2]},before=velocity;
    for(unsigned i=0;i<r->normalCount;i++)velocity=SCMotion::clip(velocity,{r->normals[i][0],r->normals[i][1],r->normals[i][2]});
    bool horizontal=fabsf(velocity.x-before.x)+fabsf(velocity.z-before.z)>.001f;
    if(horizontal){float forward=velocity.x*sinf(raw.faceAngle)+velocity.z*cosf(raw.faceAngle);libForward(id,forward);raw.forwardVelocity=forward;}
    // A contact callback can change engine momentum between ticks (a bounce).
    // Empty positional acknowledgements must not overwrite it with old telemetry.
    if(SCMotion::squared(velocity-before)>.000001f)libVelocity(id,velocity.x,velocity.y,velocity.z);
    libPosition(id,target.x,target.y,target.z);
    shiftEquipment(target.x-raw.position[0],target.y-raw.position[1],target.z-raw.position[2]);
    for(unsigned axis=0;axis<3;axis++){float delta=r->position[axis]-raw.position[axis];for(unsigned j=0;j<m->numTrianglesUsed*3;j++)m->position[j*3+axis]+=delta;raw.position[axis]=r->position[axis];}
    raw.velocity[0]=velocity.x;raw.velocity[1]=velocity.y;raw.velocity[2]=velocity.z;
    for(int k=0;k<3;k++){out->position[k]=raw.position[k];out->velocity[k]=raw.velocity[k]*SC_FIXED_HZ;}out->forwardSpeed=raw.forwardVelocity*SC_FIXED_HZ;return 1;
}
static uint32_t equipment(int32_t id,const SC_EquipmentPose*p){
    if(id!=active||id<0||!p||p->size!=sizeof(*p)||(p->rightKind!=SC_EQUIPMENT_NONE&&p->rightKind!=SC_EQUIPMENT_SWORD&&(p->rightKind<SC_EQUIPMENT_DAGGER||p->rightKind>SC_EQUIPMENT_PYROMANCY))||(p->leftKind!=SC_EQUIPMENT_NONE&&p->leftKind!=SC_EQUIPMENT_SHIELD&&p->leftKind!=SC_EQUIPMENT_PYROMANCY)||p->swing>SC_EQUIPMENT_CAST_LEFT||!std::isfinite(p->swingPhase)||!std::isfinite(p->guard)||p->swingPhase<0||p->swingPhase>1||p->guard<0||p->guard>1)return 0;
    if(!libEquipment(id,p->rightKind,p->leftKind,p->swing,p->swingPhase,p->guard))return 0;
    if(p->rightKind!=equipmentPose.rightKind||p->leftKind!=equipmentPose.leftKind||(!p->rightKind&&!p->leftKind))equipmentHands={sizeof(SC_Attachments)};
    fireAppearance=p->rightKind==SC_EQUIPMENT_PYROMANCY||p->leftKind==SC_EQUIPMENT_PYROMANCY;
    equipmentPose=*p;return 1;
}
static uint32_t attachments(int32_t id,SC_Attachments*out){
    if(id!=active||id<0||!out||out->size!=sizeof(*out))return 0;
    *out=equipmentHands;return 1;
}
static SC_CharacterAPI api={sizeof(SC_CharacterAPI),SC_ABI_VERSION,describe,initialize,shutdown,create,destroy,step,pose,resume,position,command,damage,surfaces,floorHeight,probe,contact,audio,resolveMotion,equipment,attachments};
extern "C" __declspec(dllexport) const SC_CharacterAPI* SoulsCharacterGetAPI(uint32_t abi,uint32_t size){return abi==SC_ABI_VERSION&&size==sizeof(api)?&api:nullptr;}
extern "C" __declspec(dllexport) const SC_CameraProfile* SoulsCharacterGetCameraProfile(uint32_t version,uint32_t size){return version==SC_CAMERA_PROFILE_VERSION&&size==sizeof(SC_CameraProfile)?&characterCameraProfile:nullptr;}
// SMB-inspired low, bouncing fireball. Native host retains casting and damage.
static const SC_ProjectileProfile flowerProjectile={sizeof(SC_ProjectileProfile),SC_PROJECTILE_PROFILE_VERSION,SC_PROJECTILE_FIREBALL,3.5f,1200,2400,600,3,75};
extern "C" __declspec(dllexport) const SC_ImpactProfile* SoulsCharacterGetImpactProfile(uint32_t version,uint32_t size){return version==SC_IMPACT_PROFILE_VERSION&&size==sizeof(SC_ImpactProfile)?&characterImpactProfile:nullptr;}
extern "C" __declspec(dllexport) const SC_ProjectileProfile* SoulsCharacterGetProjectileProfile(uint32_t version,uint32_t size){return version==SC_PROJECTILE_PROFILE_VERSION&&size==sizeof(SC_ProjectileProfile)?&flowerProjectile:nullptr;}

static uint32_t launcherBegin(int32_t id,const float*p,float yaw){
 if(id!=active||id<0||!cannonAssetsReady)return 0;clearEquipment(id);return libCannonBegin(id,p,yaw);
}
static void launcherCancel(int32_t id){if(id==active&&id>=0&&initialized)libCannonCancel(id);}
static uint32_t launcherQuery(int32_t id,SC_LauncherState*out){
 if(!out||out->size!=sizeof(*out)||id!=active||id<0)return 0;
 float data[7]={};out->phase=libCannonQuery(id,data);out->ticks=uint32_t(data[6]);
 memcpy(out->position,data,12);out->yaw=data[3];out->pitch=data[4];out->propLift=data[5];return 1;
}
static uint32_t launcherTexture(SC_Texture*out){
 if(!cannonAssetsReady||!out||!out->rgba||out->capacityBytes<MarioCannonAssets::atlas.size())return 0;
 memcpy(out->rgba,MarioCannonAssets::atlas.data(),MarioCannonAssets::atlas.size());return 1;
}
static const SC_LauncherAPI launcherAPI={sizeof(SC_LauncherAPI),SC_LAUNCHER_VERSION,1024,MarioCannonAssets::width,MarioCannonAssets::height,"Cannon","Bob-omb Buddy",launcherBegin,launcherCancel,launcherQuery,launcherTexture,MarioCannonAssets::mesh};
extern "C" __declspec(dllexport) const SC_LauncherAPI* SoulsCharacterGetLauncherAPI(uint32_t version,uint32_t size){return version==SC_LAUNCHER_VERSION&&size==sizeof(SC_LauncherAPI)?&launcherAPI:nullptr;}

#include "../../sdk/class_api.h"
static const SC_ClassProfile startingClass={sizeof(SC_ClassProfile),SC_CLASS_FIRECASTER,"Mario","Hero from another world.\nJumps, punches and kicks.\nFind a Fire Flower ahead.",160.f,3.14159265f};
extern "C" __declspec(dllexport) const SC_ClassProfile* SoulsCharacterGetClassProfile(uint32_t version,uint32_t size){return version==SC_CLASS_PROFILE_VERSION&&size==sizeof(startingClass)?&startingClass:nullptr;}
