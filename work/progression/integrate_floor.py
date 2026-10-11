from pathlib import Path
p=Path(__file__).resolve().parents[1]/'mariosouls.cpp';s=p.read_text()
s=s.replace('static bool loadMap(){','#include "mario_arena_floor.h"\nstatic bool loadMap(){')
s=s.replace('loadedMap=map;logf("map=', 'loadedMap=map;arenaLoad();arenaRefresh();logf("map=')
s=s.replace('if(!enabled){acc=0;return;}','if(!enabled){acc=0;return;}\n if(arenaRefresh())rebuildSurfaces();')
s=s.replace('ready=initMario();','eventFlagsSlot=scan("56 8B F1 8B 46 1C 50 A1 ? ? ? ? 32 C9",8);ready=initMario();')
p.write_text(s)
