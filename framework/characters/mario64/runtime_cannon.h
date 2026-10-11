/* Included after MarioInstance. Supply a persistent SM64 usedObj, then let
 * the existing act_in_cannon / act_shot_from_cannon run without alterations. */
#include "decomp/include/object_fields.h"
#define oBhvParams2ndByte oBehParams2ndByte
static struct Object *scCannonObject;
#define o scCannonObject
static void sc_cannon_sound(int sound) {play_sound(sound,gMarioState->marioObj->header.gfx.cameraToObject);}
#define cur_obj_play_sound_2 sc_cannon_sound
#include "sc_cannon_actions.h"
#undef cur_obj_play_sound_2
#undef o

static struct MarioInstance *sc_cannon_instance(int32_t id) {
 if(id<0||id>=s_mario_instance_pool.size||!s_mario_instance_pool.objects[id])return NULL;
 struct MarioInstance *instance=s_mario_instance_pool.objects[id];global_state_bind(instance->globalState);return instance;
}
SM64_LIB_FN uint32_t sm64_mario_cannon_begin(int32_t id,const float *position,float yaw) {
 struct MarioInstance *i=sc_cannon_instance(id);if(!i||!position||!isfinite(yaw)||i->cannonActive)return 0;
 for(int k=0;k<3;k++)if(!isfinite(position[k])||fabsf(position[k]-gMarioState->pos[k])>120)return 0;
 struct Object *c=&i->cannon;memset(c,0,sizeof(*c));
 c->oPosX=position[0];c->oPosY=position[1]-350;c->oPosZ=position[2];
 c->oMoveAngleYaw=(s16)(yaw*32768.f/3.14159265359f);
 c->oBhvParams2ndByte=((u16)c->oMoveAngleYaw)>>8;
 c->oAction=4;i->cannonActive=1;i->cannonTicks=0;i->cannonRelease=0;
 gMarioState->usedObj=c;set_mario_action(gMarioState,ACT_IN_CANNON,0);
 return 1;
}
SM64_LIB_FN void sm64_mario_cannon_cancel(int32_t id) {
 struct MarioInstance*i=sc_cannon_instance(id);if(!i||!i->cannonActive)return;
 if(gMarioState->action==ACT_IN_CANNON){set_mario_action(gMarioState,ACT_IDLE,0);gMarioState->faceAngle[0]=0;}
 gMarioState->marioObj->header.gfx.node.flags|=GRAPH_RENDER_ACTIVE;
 if(gMarioState->usedObj==&i->cannon)gMarioState->usedObj=NULL;
 i->cannonActive=0;
}
static void sc_cannon_before_tick(struct MarioInstance*i) {
 if(!i->cannonActive)return;
 if(gMarioState->action!=ACT_IN_CANNON&&gMarioState->action!=ACT_SHOT_FROM_CANNON){
  gMarioState->marioObj->header.gfx.node.flags|=GRAPH_RENDER_ACTIVE;
  if(gMarioState->usedObj==&i->cannon)gMarioState->usedObj=NULL;
  i->cannonActive=0;return;
 }
 if(gMarioState->action!=ACT_IN_CANNON)return;
 scCannonObject=&i->cannon;int previous=scCannonObject->oAction;
 switch(previous){case 4:opened_cannon_act_4();break;case 6:opened_cannon_act_6();break;case 5:opened_cannon_act_5();break;}
 if(previous!=scCannonObject->oAction)scCannonObject->oTimer=0;else scCannonObject->oTimer++;
 i->cannonTicks++;
}
SM64_LIB_FN uint32_t sm64_mario_cannon_query(int32_t id,float*out) {
 struct MarioInstance*i=sc_cannon_instance(id);if(!i||!out||!i->cannonActive)return 0;
 struct MarioState*m=gMarioState;
 uint32_t phase=m->action==ACT_IN_CANNON?(m->actionState==2?2:1):m->action==ACT_SHOT_FROM_CANNON?3:0;
 out[0]=m->pos[0];out[1]=m->pos[1];out[2]=m->pos[2];
 out[3]=(phase==1?(s16)i->cannon.oMoveAngleYaw:m->faceAngle[1])*(3.14159265359f/32768.f);out[4]=(phase==1?(s16)i->cannon.oMoveAnglePitch:m->faceAngle[0])*(3.14159265359f/32768.f);
 out[5]=i->cannon.oPosY;out[6]=(float)i->cannonTicks;
 return phase;
}
