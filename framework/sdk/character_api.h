#ifndef SC_CHARACTER_API_H
#define SC_CHARACTER_API_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* C ABI, caller-owned buffers, no STL or allocator ownership across DLLs.
 * All positions/geometry/collision use local centimetres, Y up, native X/Z.
 * Velocity is centimetres/second. Yaw is radians: 0 faces +Z, positive toward +X.
 * Fixed simulation callbacks are 30 Hz; adapters resample their own engine if needed.
 * Input X is right, Y is forward relative to camera. No host pointers are exposed. */
#define SC_ABI_VERSION 4u
#define SC_FIXED_HZ 30u
#define SC_MAX_ATTACKS 32u
#define SC_MAX_TRIANGLES 16384u
#define SC_CAP_AUDIO 1u
#define SC_CAP_ITEM_PROP 2u
#define SC_CAP_TEXTURE_MODULATE 4u
#define SC_CAP_ALPHA_TEST 8u
#define SC_CAP_EQUIPMENT 16u
#define SC_CAP_DEATH_POSE 32u
#define SC_CAP_MELEE_FAMILIES 64u
#define SC_CAP_NATIVE_COMBAT 128u
#define SC_GROUNDED 1u
#define SC_CAN_INTERACT 2u
#define SC_AUTO_ATTACK_ELIGIBLE 4u
#define SC_ATTACK_ACTIVE 8u
#define SC_CAN_EQUIP_ACTION 16u
#define SC_ATTACK_AUTOMATIC 1u
#define SC_ATTACK_RADIAL 2u
#define SC_ATTACK_NO_OBJECTS 4u
#define SC_CONTACT_ATTACK 1u
#define SC_CONTACT_AUTOMATIC 2u

typedef struct SC_Input {
    float camLookX, camLookZ, moveX, moveY;
    uint8_t jump, attack, crouch, guard;
} SC_Input;
typedef struct SC_Surface { int32_t vertices[3][3]; uint32_t material; } SC_Surface;
typedef struct SC_Mesh {
    float *position, *normal, *color, *uv;
    uint32_t capacityTriangles, numTrianglesUsed;
} SC_Mesh;
typedef struct SC_Texture { uint8_t *rgba; uint32_t capacityBytes; } SC_Texture;
typedef struct SC_State {
    float position[3], velocity[3], faceAngle, forwardSpeed, floorHeight;
    uint32_t flags;
    int32_t attackId, interruptedAttackId;
    uint32_t landingRecoveryTicks;
    /* Opaque diagnostic values. Host behaviour must not depend on these. */
    uint32_t rawAction, rawFlags, rawActionArg, rawActionTimer;
    int32_t rawAnimation, rawAnimationFrame;
} SC_State;
typedef struct SC_Attack {
    char name[32];
    float multiplier, poise;
    uint32_t staminaCost, flags, activeTicks, maxTargets;
    float rangeCm, minHeightCm, maxHeightCm;
    /* Automatic contact eligibility, relative enemy feet. */
    float automaticRadiusCm, automaticMinHeightCm, automaticMaxHeightCm;
} SC_Attack;

/* Presentation is independent of locomotion speed and attack reach. Zero body
 * dimensions keep the host's original body. Heights include both rounded ends.
 * Fall reduction applies only to landing damage, never enemy/environment hits. */
typedef struct SC_Body {
    float modelScale;
    float radiusCm, heightCm, hurtRadiusCm, hurtHeightCm;
    float fallDamageScale, fallDamageMaxHPFraction;
    uint32_t nonlethalFalls;
} SC_Body;
typedef struct SC_Descriptor {
    uint32_t size, abiVersion;
    char id[48], displayName[64];
    uint32_t maxTriangles, textureWidth, textureHeight, capabilities, audioRate;
    uint32_t attackCount;
    int32_t automaticAttackId;
    float basePower, upgradePower, dexterityWeight;
    SC_Attack attacks[SC_MAX_ATTACKS];
    SC_Body body;
} SC_Descriptor;
typedef void (*SC_Log)(const char *message);
typedef struct SC_Init {
    uint32_t size;
    const char *assetPath, *runtimePath;
    SC_Log log;
} SC_Init;
enum SC_PoseKind { SC_POSE_NEUTRAL, SC_POSE_CLIMB, SC_POSE_DRINK, SC_POSE_REST,
    SC_POSE_DOOR, SC_POSE_WALK, SC_POSE_DOWNED, SC_POSE_TALK, SC_POSE_STAGGER,
    SC_POSE_DEATH, SC_POSE_NATIVE_COMBAT };
typedef struct SC_Pose {
    uint32_t kind;
    float position[3], yaw, phase, travelCm;
} SC_Pose;
enum SC_Command { SC_COMMAND_IDLE, SC_COMMAND_FALL };

/* Resolved host position and collision planes, local cm; never a simulation tick.
 * Update momentum only into the supplied unit normals; preserve tangential motion,
 * the current action and animation phase. Shift the existing mesh coherently. */
typedef struct SC_MotionResolution {
    uint32_t size,generation,sequence;
    float position[3];
    uint32_t normalCount;
    float normals[4][3];
} SC_MotionResolution;

/* Host-owned equipment action, presentation only; never advance simulation.
 * Kind values carry no native inventory pointers or addresses. The host retains
 * native equipment rules, stamina, damage and guard state. */
enum SC_EquipmentKind { SC_EQUIPMENT_NONE, SC_EQUIPMENT_SWORD, SC_EQUIPMENT_SHIELD,
    SC_EQUIPMENT_DAGGER, SC_EQUIPMENT_CURVED_SWORD, SC_EQUIPMENT_GREATSWORD,
    SC_EQUIPMENT_AXE, SC_EQUIPMENT_HAMMER };
/* Special values require SC_CAP_NATIVE_COMBAT. Native combat posing preserves
 * the equipment layer; the adapter never performs damage or locomotion. */
enum SC_EquipmentAction { SC_EQUIPMENT_READY, SC_EQUIPMENT_LIGHT_1, SC_EQUIPMENT_LIGHT_2,
    SC_EQUIPMENT_BACKSTAB, SC_EQUIPMENT_RIPOSTE, SC_EQUIPMENT_PARRY };
typedef struct SC_EquipmentPose {
    uint32_t size, rightKind, leftKind, swing;
    float swingPhase, guard;
} SC_EquipmentPose;
/* Column-major rigid transforms: orthonormal basis, local-centimetre translation.
 * +Z is sword blade direction / shield front, +Y is up. They describe the SAME
 * mesh frame returned by step/pose/resolveMotion. valid bits: 1 right, 2 left. */
typedef struct SC_Attachments {
    uint32_t size, valid;
    float right[16], left[16];
} SC_Attachments;

typedef struct SC_CharacterAPI {
    uint32_t size, abiVersion;
    const SC_Descriptor *(*describe)(void);
    uint32_t (*initialize)(const SC_Init *, SC_Texture *);
    void (*shutdown)(void);
    int32_t (*create)(float x,float y,float z,float yaw);
    void (*destroy)(int32_t);
    uint32_t (*step)(int32_t,const SC_Input *,SC_State *,SC_Mesh *);
    uint32_t (*pose)(int32_t,const SC_Pose *,SC_State *,SC_Mesh *);
    void (*resume)(int32_t,float,float,float,float);
    void (*setPosition)(int32_t,float,float,float);
    void (*command)(int32_t,uint32_t);
    void (*nativeDamage)(int32_t,uint32_t severity);
    void (*surfaces)(const SC_Surface *,uint32_t count,uint32_t dynamicContacts);
    float (*floorHeight)(float,float,float);
    uint32_t (*probeContact)(int32_t,float,float,float);
    void (*commitContact)(int32_t,float,float,float,float radiusCm);
    /* Return stereo sample FRAMES, never bytes. capacityFrames is a hard bound. */
    uint32_t (*audio)(uint32_t queuedFrames,uint32_t desiredFrames,int16_t *stereo,uint32_t capacityFrames);
    uint32_t (*resolveMotion)(int32_t,const SC_MotionResolution *,SC_State *,SC_Mesh *);
    uint32_t (*equipment)(int32_t,const SC_EquipmentPose *);
    uint32_t (*attachments)(int32_t,SC_Attachments *);
} SC_CharacterAPI;
typedef const SC_CharacterAPI *(*SC_GetAPI)(uint32_t hostAbi,uint32_t hostApiSize);

#ifdef __cplusplus
}
#endif
#endif
