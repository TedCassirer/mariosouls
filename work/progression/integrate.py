from pathlib import Path
w=Path(__file__).resolve().parents[1];p=w/'mariosouls.cpp';s=p.read_text()
s=s.replace('#include "interaction_state.h"','#include "interaction_state.h"\n#include "mario_combat_policy.h"\nstatic void combatReset();\nstatic void combatSettings();')
s=s.replace('static void (*sm_health)(int32_t,uint16_t);','static void (*sm_health)(int32_t,uint16_t);\nstatic void (*sm_host_health)(int32_t,int32_t);\nstatic uint32_t (*sm_probe)(int32_t,float,float,float);')
s=s.replace('static DamageFn nativeDamage=nullptr;','static DamageFn nativeDamage=nullptr;\n#include "mario_native_combat.h"')
s=s.replace('API(sm_hurt,"sm64_mario_take_damage");API(sm_health,"sm64_set_mario_health");','API(sm_hurt,"sm64_mario_take_damage");API(sm_health,"sm64_set_mario_health");API(sm_host_health,"sm64_mario_host_health");API(sm_probe,"sm64_mario_probe_attack");')
s=s.replace('nativeDamage=(DamageFn)0xd65dc0;','nativeDamage=(DamageFn)0xd65dc0;\n if(nativeDamage)marioInstallDamage();combatSettings();')
s=s.replace('static void releaseControl(bool keepMario=false){','static void releaseControl(bool keepMario=false){\n combatReset();')
s=s.replace('mario=sm_create(0,40,0);if(mario<0)return false;','mario=sm_create(0,40,0);if(mario<0)return false;combatReset();sm_host_health(mario,0);')
start=s.index('static void combat(){');end=s.index('static void update(){',start)
s=s[:start]+'#include "mario_combat_runtime.h"\n'+s[end:]
old='int hp=read<int>(player+0x2d4);if(hp<lastHP&&hp>0){sm_hurt(mario,1,0,state.position[0]+sinf(state.faceAngle)*100,state.position[1],state.position[2]+cosf(state.faceAngle)*100);logf("native damage %d -> %d",lastHP,hp);}lastHP=hp;sm_health(mario,0x880);'
assert old in s
s=s.replace(old,'int hp=read<int>(player+0x2d4);if(hp<lastHP&&hp>0){int lost=lastHP-hp;sm_host_health(mario,lost>=read<int>(player+0x2d8)/5?2:1);logf("native damage %d -> %d; host-confirmed hurt, no extra i-frames",lastHP,hp);}else sm_host_health(mario,0);lastHP=hp;')
s=s.replace('lastLog=GetTickCount();renderMode=','lastLog=GetTickCount();combatSettings();renderMode=')
s=s.replace('harnessInput(input);uint32_t old=','harnessInput(input);combatInput(input);uint32_t old=')
p.write_text(s)
