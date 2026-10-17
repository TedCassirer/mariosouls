// Local host actions sample the owned ROM's skeleton and blend joint angles.
// No guest door/pole object is fabricated and no guest gameplay action is run.
static void host_sample(struct Animation *a, int frame, s16 out[63]) {
    if(frame<0)frame=0;if(frame>=a->loopEnd)frame=a->loopEnd-1;
    for(int i=0;i<63;i++){int count=a->index[i*2],offset=a->index[i*2+1];out[i]=a->values[offset+(frame<count?frame:count-1)];}
    if(a->flags&ANIM_FLAG_HOR_TRANS)out[0]=out[2]=0;
    else if(a->flags&ANIM_FLAG_VERT_TRANS)out[1]=0;
    else if(a->flags&ANIM_FLAG_6)out[0]=out[1]=out[2]=0;
}
static void host_render(struct MarioInstance *instance,struct SM64MarioGeometryBuffers *buffers,int key,s16 *target) {
    struct AnimInfo *info=&gMarioState->marioObj->header.gfx.animInfo;
    if(!target&&instance->poseKey==0&&!instance->blendTicks){
        gfx_adapter_bind_output_buffers(buffers);geo_process_root_hack_single_node(s_mario_graph_node);
        host_sample(info->curAnim,info->animFrame,instance->lastPose);instance->poseValid=true;return;
    }
    struct AnimInfo saved=*info;
    s16 desired[63];
    if(target)memcpy(desired,target,sizeof(desired));
    else {
        saved.animFrame=geo_update_animation_frame(info,&saved.animFrameAccelAssist);
        saved.animTimer=gAreaUpdateCounter;
        host_sample(saved.curAnim,saved.animFrame,desired);
    }
    if(instance->poseKey!=key){
        instance->poseKey=key;instance->blendTicks=instance->poseValid?6:0;
        memcpy(instance->blendFrom,instance->lastPose,sizeof(instance->lastPose));
    }
    float alpha=instance->blendTicks?1.f-(instance->blendTicks-1)/6.f:1.f;
    for(int i=0;i<63;i++){
        int delta=i<3?(int)desired[i]-instance->blendFrom[i]:(s16)(desired[i]-instance->blendFrom[i]);
        instance->lastPose[i]=instance->blendTicks?(s16)(instance->blendFrom[i]+delta*alpha):desired[i];
        instance->poseIndices[i*2]=1;instance->poseIndices[i*2+1]=i;
    }
    if(instance->blendTicks)--instance->blendTicks;
    instance->poseValid=true;
    struct Animation pose=*saved.curAnim;
    pose.flags=0;pose.startFrame=pose.loopStart=0;pose.loopEnd=1;
    pose.values=instance->lastPose;pose.index=instance->poseIndices;
    info->curAnim=&pose;info->animFrame=0;info->animFrameAccelAssist=0;info->animAccel=0;info->animTimer=gAreaUpdateCounter;
    gfx_adapter_bind_output_buffers(buffers);geo_process_root_hack_single_node(s_mario_graph_node);
    *info=saved;
}

SM64_LIB_FN void sm64_mario_host_pose_tick(int32_t id,float x,float y,float z,float facing,int32_t animation,float phase,int32_t style,struct SM64MarioState *out,struct SM64MarioGeometryBuffers *buffers) {
    if(id<0||id>=s_mario_instance_pool.size||!s_mario_instance_pool.objects[id]||animation<0||animation>0xd0)return;
    struct MarioInstance *instance=s_mario_instance_pool.objects[id];global_state_bind(instance->globalState);
    struct MarioState *m=gMarioState;
    vec3f_set(m->pos,x,y,z);vec3f_set(m->vel,0,0,0);m->forwardVel=0;m->action=ACT_IDLE;
    vec3s_set(m->faceAngle,0,(s16)(facing*32768.f/3.14159265359f),0);
    vec3f_copy(m->marioObj->header.gfx.pos,m->pos);vec3s_copy(m->marioObj->header.gfx.angle,m->faceAngle);
    vec3f_set(m->marioObj->header.gfx.scale,1,1,1);m->marioObj->header.gfx.throwMatrix=NULL;
    mario_reset_bodystate(m);mario_update_hitbox_and_cap_model(m);
    m->marioBodyState->action=ACT_IDLE;m->marioBodyState->punchState=0;
    vec3s_set(m->marioBodyState->headAngle,0,0,0);vec3s_set(m->marioBodyState->torsoAngle,0,0,0);
    set_mario_animation(m,animation);
    struct Animation *a=m->marioObj->header.gfx.animInfo.curAnim;s16 target[63];
    int frame=m->marioObj->header.gfx.animInfo.animFrame;
    if(style&1)frame=(int)(fmaxf(0,fminf(1,phase))*(a->loopEnd-1));
    else if(style&2){frame=(int)phase%a->loopEnd;if(frame<0)frame+=a->loopEnd;}
    else if(style&4)frame=22;
    else {frame++;if(frame>=a->loopEnd)frame=a->loopStart;}
    m->marioObj->header.gfx.animInfo.animFrame=frame;
    host_sample(a,frame,target);
    if(style&4){
        // Authored right-arm grip solved against Mario's actual ROM skeleton.
        // Keep planted idle legs and bring the flask neck to the mouth.
        struct MarioAnimation idle={0};load_mario_animation(&idle,0xc3);s16 base[63];host_sample(idle.targetAnim,0,base);
        float amount=fmaxf(0,fminf(1,phase));
        amount=amount*amount*(3-2*amount);
        static const s16 drink[9]={5532,30141,-7657,224,3930,-11138,0,852,-718};
        for(int i=0;i<63;i++){
            target[i]=i>=30&&i<39?(s16)(base[i]+(s16)(drink[i-30]-base[i])*amount):base[i];
        }
    }
    host_render(instance,buffers,1+animation+(style<<9),target);gAreaUpdateCounter++;
    memset(out,0,sizeof(*out));vec3f_copy(out->position,m->pos);out->faceAngle=facing;
    out->action=m->action;out->health=m->health;out->animID=animation;out->animFrame=frame;out->flags=m->flags;
}

SM64_LIB_FN void sm64_mario_host_resume(int32_t id,float x,float y,float z,float facing) {
    if(id<0||id>=s_mario_instance_pool.size||!s_mario_instance_pool.objects[id])return;
    global_state_bind(((struct MarioInstance*)s_mario_instance_pool.objects[id])->globalState);
    struct MarioState *m=gMarioState;vec3f_set(m->pos,x,y,z);vec3f_set(m->vel,0,0,0);
    m->forwardVel=0;m->faceAngle[1]=(s16)(facing*32768.f/3.14159265359f);
    m->floorHeight=find_floor(x,y+30,z,&m->floor);m->ceil=NULL;m->wall=NULL;m->marioObj->platform=NULL;
    m->input=0;gController.buttonPressed=gController.buttonDown=0;
    set_mario_action(m,m->floor&&fabsf(y-m->floorHeight)<35?ACT_IDLE:ACT_FREEFALL,0);
    m->marioObj->header.gfx.animInfo.animID=-1;
}

extern float sm64_host_right_hand[4][4];
SM64_LIB_FN void sm64_mario_hand_matrix(float *out){memcpy(out,sm64_host_right_hand,sizeof(float)*16);}
