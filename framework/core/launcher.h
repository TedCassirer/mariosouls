#pragma once
#include "../sdk/launcher_api.h"
#include <cmath>
namespace SCLauncher {
inline bool valid(const SC_LauncherAPI*a){return a&&a->size==sizeof(*a)&&a->version==SC_LAUNCHER_VERSION&&a->maxTriangles>0&&a->maxTriangles<=4096&&a->textureWidth>0&&a->textureWidth<=2048&&a->textureHeight>0&&a->textureHeight<=2048&&a->name&&a->operatorName&&a->begin&&a->cancel&&a->query&&a->texture&&a->mesh;}
inline bool valid(const SC_LauncherState&s){
 if(s.size!=sizeof(s)||s.phase>SC_LAUNCHER_FLIGHT||!std::isfinite(s.yaw)||!std::isfinite(s.pitch)||!std::isfinite(s.propLift))return false;
 for(float p:s.position)if(!std::isfinite(p)||fabsf(p)>30000)return false;
 return true;
}
inline bool withinReach(const float*a,const float*b,float radius,float height){
 float x=a[0]-b[0],y=a[1]-b[1],z=a[2]-b[2];return std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&fabsf(y)<height&&x*x+z*z<radius*radius;
}
}
