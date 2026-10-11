#pragma once
#include "../../sdk/character_api.h"
// Verified PC parameter pairs, not item-ID ranges. Special heavy/ranged classes
// require their own fitted actions and are intentionally not classified here.
inline uint32_t equipmentFamily(int category,int animation){
 if(category==0&&animation==20)return SC_EQUIPMENT_DAGGER;
 if(category==1&&animation==23)return SC_EQUIPMENT_SWORD;
 if(category==1&&animation==25)return SC_EQUIPMENT_GREATSWORD;
 if(category==3&&animation==28)return SC_EQUIPMENT_CURVED_SWORD;
 if(category==4&&animation==30)return SC_EQUIPMENT_AXE;
 if(category==5&&animation==33)return SC_EQUIPMENT_HAMMER;
 return SC_EQUIPMENT_NONE;
}
