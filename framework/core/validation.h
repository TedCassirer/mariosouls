#pragma once
#include "../sdk/character_api.h"
#include <cmath>
#include <cstring>
#include <cctype>
namespace SC {
inline bool identifier(const char *s,size_t max){
    size_t i=0;for(;i<max&&s[i];++i)if(!(s[i]>='a'&&s[i]<='z')&&!(s[i]>='0'&&s[i]<='9')&&s[i]!='_')return false;
    return i>0&&i<max;
}
inline bool descriptor(const SC_Descriptor *d){
    if(!d||d->size!=sizeof(*d)||d->abiVersion!=SC_ABI_VERSION||!identifier(d->id,sizeof(d->id))||!memchr(d->displayName,0,sizeof(d->displayName)))return false;
    if(!d->maxTriangles||d->maxTriangles>SC_MAX_TRIANGLES||!d->textureWidth||d->textureWidth>4096||!d->textureHeight||d->textureHeight>4096)return false;
    if(d->attackCount>SC_MAX_ATTACKS||d->automaticAttackId< -1||d->automaticAttackId>=int(d->attackCount))return false;
    if(!std::isfinite(d->basePower)||d->basePower<1||!std::isfinite(d->upgradePower)||d->upgradePower<0||!std::isfinite(d->dexterityWeight)||d->dexterityWeight<0)return false;
    if((d->capabilities&SC_CAP_AUDIO)&&(d->audioRate<8000||d->audioRate>192000))return false;
    if(d->capabilities&~(SC_CAP_AUDIO|SC_CAP_ITEM_PROP))return false;
    for(unsigned i=0;i<d->attackCount;i++){
        const auto&a=d->attacks[i];if(!identifier(a.name,sizeof(a.name))||!std::isfinite(a.multiplier)||a.multiplier<0||a.multiplier>10||!std::isfinite(a.poise)||a.poise<0||a.staminaCost>1000||a.maxTargets<1||a.maxTargets>64||!std::isfinite(a.rangeCm)||a.rangeCm<=0||a.rangeCm>1000)return false;
        for(unsigned j=0;j<i;j++)if(!strcmp(a.name,d->attacks[j].name))return false;
        if(a.activeTicks>300||!std::isfinite(a.minHeightCm)||!std::isfinite(a.maxHeightCm)||a.minHeightCm>=a.maxHeightCm||a.flags&~(SC_ATTACK_AUTOMATIC|SC_ATTACK_RADIAL|SC_ATTACK_NO_OBJECTS))return false;
        if(!std::isfinite(a.automaticRadiusCm)||!std::isfinite(a.automaticMinHeightCm)||!std::isfinite(a.automaticMaxHeightCm))return false;
        if(a.flags&SC_ATTACK_AUTOMATIC){if(int(i)!=d->automaticAttackId||a.automaticRadiusCm<=0||a.automaticRadiusCm>1000||a.automaticMinHeightCm>=a.automaticMaxHeightCm)return false;}
        if(int(i)==d->automaticAttackId&&!(a.flags&SC_ATTACK_AUTOMATIC))return false;
    }return true;
}
inline bool api(const SC_CharacterAPI *a){return a&&a->abiVersion==SC_ABI_VERSION&&a->size==sizeof(*a)&&a->describe&&a->initialize&&a->shutdown&&a->create&&a->destroy&&a->step&&a->pose&&a->resume&&a->setPosition&&a->command&&a->nativeDamage&&a->surfaces&&a->floorHeight&&a->probeContact&&a->commitContact&&a->audio;}
inline bool frame(const SC_State&s,const SC_Mesh&m,const SC_Descriptor&d){
    if(m.numTrianglesUsed>m.capacityTriangles||m.numTrianglesUsed>d.maxTriangles||s.attackId< -1||s.attackId>=int(d.attackCount)||s.interruptedAttackId< -1||s.interruptedAttackId>=int(d.attackCount)||s.landingRecoveryTicks>300)return false;
    for(int i=0;i<3;i++)if(!std::isfinite(s.position[i])||!std::isfinite(s.velocity[i]))return false;
    if(!std::isfinite(s.faceAngle)||!std::isfinite(s.floorHeight)||!std::isfinite(s.forwardSpeed))return false;
    for(unsigned i=0;i<m.numTrianglesUsed*9;i++)if(!std::isfinite(m.position[i])||!std::isfinite(m.normal[i])||!std::isfinite(m.color[i]))return false;
    for(unsigned i=0;i<m.numTrianglesUsed*6;i++)if(!std::isfinite(m.uv[i]))return false;
    return true;
}
}
