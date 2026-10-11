/* Original, adapter-owned equipment layer in the generated libsm64 copy.
 * Render poses only change local arm rotations, never the animation clock or
 * movement. An explicit armed-idle policy prevents sleeping with a held item. */
static struct MarioInstance *scEquipmentRendering;

static float sc_equipment_unit(float v) { return fmaxf(0.f,fminf(1.f,v)); }
static float sc_equipment_smooth(float v) { v=sc_equipment_unit(v);return v*v*(3.f-2.f*v); }
static void sc_equipment_quaternion(const s16 *a,float *q) {
    const float scale=3.14159265359f/65536.f;
    float sx=sinf(a[0]*scale),cx=cosf(a[0]*scale),sy=sinf(a[1]*scale),cy=cosf(a[1]*scale),sz=sinf(a[2]*scale),cz=cosf(a[2]*scale);
    q[0]=sx*cy*cz-cx*sy*sz;q[1]=cx*sy*cz+sx*cy*sz;q[2]=cx*cy*sz-sx*sy*cz;q[3]=cx*cy*cz+sx*sy*sz;
}
static void sc_equipment_rotation(const s16 *from,const s16 *to,float t,s16 *out) {
    if(t<=0){memcpy(out,from,sizeof(s16)*3);return;}if(t>=1){memcpy(out,to,sizeof(s16)*3);return;}
    float a[4],b[4],q[4],dot=0;sc_equipment_quaternion(from,a);sc_equipment_quaternion(to,b);
    for(int i=0;i<4;i++)dot+=a[i]*b[i];
    if(dot<0){dot=-dot;for(int i=0;i<4;i++)b[i]=-b[i];}
    float left=1-t,right=t;
    if(dot<.9995f){float angle=acosf(fminf(1.f,dot)),denom=sinf(angle);left=sinf((1-t)*angle)/denom;right=sinf(t*angle)/denom;}
    float length=0;for(int i=0;i<4;i++){q[i]=a[i]*left+b[i]*right;length+=q[i]*q[i];}length=sqrtf(length);for(int i=0;i<4;i++)q[i]/=length;
    float x=q[0],y=q[1],z=q[2],w=q[3],scale=32768.f/3.14159265359f;
    out[0]=(s16)(atan2f(2*(w*x+y*z),1-2*(x*x+y*y))*scale);
    out[1]=(s16)(asinf(fmaxf(-1.f,fminf(1.f,2*(w*y-z*x))))*scale);
    out[2]=(s16)(atan2f(2*(w*z+x*y),1-2*(y*y+z*z))*scale);
}

/* Authored arm keys fitted against the owned-ROM skeleton. The nine channels
 * are upper arm, forearm, wrist XYZ. They are not native Dark Souls poses. */
static const s16 scSwordKeys[4][9]={
    {-6463,-7897,-30470,-38,4,37,-26207,7206,26008},
    {17213,-11649,-12774,-49,255,8651,12615,-8195,19571},
    {3158,-3664,32622,-406,-6,12,-23609,12371,-25574},
    {-10751,2582,-32731,226,3,9,20675,412,23390}
};
static const s16 scChopKeys[4][9]={
    {-6463,-7897,-30470,-38,4,37,-26207,7206,26008},
    {17213,-11649,-17774,-49,255,8651,12615,-8195,19571},
    {3158,-3664,32622,-406,-6,12,-23609,12371,-25574},
    {-10751,2582,-30731,226,3,9,20675,412,23390}
};
static const s16 scShieldKeys[2][9]={
    {4344,6986,32649,35,-34,6614,-27706,10303,28385},
    {-1054,-4567,30852,-28,-10,12,30580,12574,29400}
};

static void sc_equipment_before_tick(struct MarioInstance *instance) {
    if(!instance->equipmentRight&&!instance->equipmentLeft)return;
    struct MarioState *m=gMarioState;
    if(m->action==ACT_IDLE){
        // Preserve the ordinary three head-turn animations. Only inhibit their
        // ten-cycle transition into sleep while either hand carries equipment.
        if(m->actionTimer>=9)m->actionTimer=8;
        if(m->actionState==3)m->actionState=0;
    }else if(m->action==ACT_START_SLEEPING||m->action==ACT_SLEEPING){
        // Use the guest's normal twenty-tick wake animation, not an idle reset
        // every frame. Safe-action eligibility remains false until it finishes.
        set_mario_action(m,ACT_WAKING_UP,m->action==ACT_SLEEPING?m->actionState:0);
    }
}

static void sc_equipment_begin(struct MarioInstance *instance) {
    instance->equipmentValid=0;
    scEquipmentRendering=NULL;
    if(!instance->equipmentRight&&!instance->equipmentLeft)return;
    if(gMarioState->action&(ACT_FLAG_INTANGIBLE|ACT_FLAG_INVULNERABLE|ACT_FLAG_SWIMMING))return;
    if(gMarioState->action==ACT_WAKING_UP)return;
    scEquipmentRendering=instance;
}
static void sc_equipment_end(void) { scEquipmentRendering=NULL; }

/* Called from the animated-part renderer after consuming the animation's
 * channels; changing the reader position here would corrupt the whole model. */
void sc_mario_equipment_rotation(int attributeOffset,s16 *rotation) {
    struct MarioInstance *instance=scEquipmentRendering;
    if(!instance)return;
    int channel=attributeOffset/2-3;
    /* Let genuine Mario attacks keep their complete authored guest pose. */
    if((gMarioState->action&ACT_FLAG_ATTACKING)&&!instance->equipmentSwing)return;
    // The guest tree calls X-positive the left arm. The host's left-handed
    // view presents that glove on the character's right. Mirrored authored
    // joint poses bind native equipment to the glove seen on the proper side.
    if(instance->equipmentRight!=0&&channel>=18&&channel<27){
        int first=0,second=0;float blend=0;
        if(instance->equipmentSwing){
            const float times[5]={0.f,.20f,.37f,.55f,.90f};
            const int forward[5]={0,1,2,3,0},reverse[5]={0,3,2,1,0};
            const int *keys=instance->equipmentSwing==2&&instance->equipmentRight<6?reverse:forward;
            float phase=instance->equipmentPhase;
            for(int k=0;k<4;k++)if(phase>=times[k]&&phase<times[k+1]){
                first=keys[k];second=keys[k+1];blend=sc_equipment_smooth((phase-times[k])/(times[k+1]-times[k]));break;
            }
        }
        const s16 (*pose)[9]=instance->equipmentRight>=6?scChopKeys:scSwordKeys;
        sc_equipment_rotation(pose[first]+channel-18,pose[second]+channel-18,blend,rotation);
    }else if(instance->equipmentLeft==2&&channel>=30&&channel<39){
        float blend=sc_equipment_smooth(instance->equipmentGuard);
        sc_equipment_rotation(scShieldKeys[0]+channel-30,scShieldKeys[1]+channel-30,blend,rotation);
    }
}

/* Both sockets come from the exact graph traversal that emitted this mesh.
 * The graph's intrinsic 0.25 scale is removed from basis vectors only; grip
 * translation remains in the same local centimetres as the Mario mesh. */
void sc_mario_equipment_hand(int attributeOffset,const float *matrix) {
    struct MarioInstance *instance=scEquipmentRendering;
    if(!instance)return;
    unsigned bit;float *out;
    if(attributeOffset==54&&instance->equipmentRight!=0){bit=1;out=instance->equipmentRightHand;}
    else if(attributeOffset==78&&instance->equipmentLeft==2){bit=2;out=instance->equipmentLeftHand;}
    else return;
    memset(out,0,sizeof(float)*16);
    for(int column=0;column<3;column++){
        const float *axis=matrix+column*4;
        float length=sqrtf(axis[0]*axis[0]+axis[1]*axis[1]+axis[2]*axis[2]);
        if(!isfinite(length)||length<.001f)return;
        // Mirroring the arm preserves its local +X bone/grip extension but
        // reverses glove Y. A rigid half-roll restores the prop's up direction.
        for(int row=0;row<3;row++)out[column*4+row]=axis[row]/length*(column<2?-1.f:1.f);
    }
    for(int row=0;row<3;row++){
        out[12+row]=matrix[12+row]+matrix[row]*40.f;
        if(!isfinite(out[12+row]))return;
    }
    out[15]=1.f;instance->equipmentValid|=bit;
}

SM64_LIB_FN uint32_t sm64_mario_equipment(int32_t id,uint32_t right,uint32_t left,uint32_t swing,float phase,float guard) {
    if(id<0||id>=s_mario_instance_pool.size||!s_mario_instance_pool.objects[id]||(right!=0&&right!=1&&(right<3||right>7))||(left!=0&&left!=2)||swing>2||!isfinite(phase)||!isfinite(guard))return 0;
    struct MarioInstance *instance=s_mario_instance_pool.objects[id];
    instance->equipmentRight=right;instance->equipmentLeft=left;
    instance->equipmentSwing=right?swing:0;instance->equipmentPhase=sc_equipment_unit(phase);instance->equipmentGuard=left?sc_equipment_unit(guard):0;
    if(!right&&!left)instance->equipmentValid=0;
    return 1;
}
SM64_LIB_FN uint32_t sm64_mario_equipment_hands(int32_t id,float *right,float *left) {
    if(id<0||id>=s_mario_instance_pool.size||!s_mario_instance_pool.objects[id]||!right||!left)return 0;
    struct MarioInstance *instance=s_mario_instance_pool.objects[id];
    if(instance->equipmentValid&1)memcpy(right,instance->equipmentRightHand,sizeof(float)*16);
    if(instance->equipmentValid&2)memcpy(left,instance->equipmentLeftHand,sizeof(float)*16);
    return instance->equipmentValid;
}
