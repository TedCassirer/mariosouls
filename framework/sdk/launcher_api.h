#pragma once
#include "character_api.h"
/* Optional ABI: guest-owned launcher simulation + ROM-derived world props.
 * No native pointers. All positions are local cm; angles radians, pitch up.
 * begin is called only from a consumed world interaction; tick remains step().
 * query and mesh do not advance simulation. cancel is idempotent. */
#define SC_LAUNCHER_VERSION 1u
#define SC_LAUNCHER_OPENING 1u
#define SC_LAUNCHER_AIMING 2u
#define SC_LAUNCHER_FLIGHT 3u
#define SC_LAUNCHER_BASE 1u
#define SC_LAUNCHER_BARREL 2u
#define SC_LAUNCHER_OPERATOR 3u
#define SC_LAUNCHER_LID 4u
typedef struct SC_LauncherState {
 uint32_t size,phase,ticks;
 float position[3],yaw,pitch,propLift;
} SC_LauncherState;
typedef struct SC_LauncherAPI {
 uint32_t size,version,maxTriangles,textureWidth,textureHeight;
 const char *name,*operatorName;
 uint32_t (*begin)(int32_t id,const float*position,float yaw);
 void (*cancel)(int32_t id);
 uint32_t (*query)(int32_t id,SC_LauncherState*state);
 uint32_t (*texture)(SC_Texture*texture);
 uint32_t (*mesh)(uint32_t part,float frame,float viewYaw,SC_Mesh*mesh);
} SC_LauncherAPI;
typedef const SC_LauncherAPI* (*SC_GetLauncherAPI)(uint32_t version,uint32_t size);
