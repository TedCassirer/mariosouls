#pragma once
#include "../../core/hazard.h"
// MSB CollisionHitFilter: 13 death camera, 14 lethal fall, 15 kill plane.
// Camera surfaces must not become floors. Lethal surface contact is resolved
// before guest gameplay can continue, independently of landing damage.
static bool hazardTrigger(unsigned filter){return filter>=13&&filter<=15;}
static bool hazardLethal(unsigned filter){return filter==14||filter==15;}
static std::vector<SCHazard::Triangle> mapHazards;
static SCHazard::Point hazardPrevious{};
static bool hazardPreviousValid=false;
static void hazardStart(Vec p){hazardPrevious={p.x,p.y,p.z};hazardPreviousValid=true;}
static bool hazardObserve(Vec p){
 SCHazard::Point to={p.x,p.y,p.z},from=hazardPreviousValid?hazardPrevious:to;
 hazardPrevious=to;hazardPreviousValid=true;
 for(const auto& t:mapHazards)if(SCHazard::touches(from,to,t))return true;
 return false;
}
