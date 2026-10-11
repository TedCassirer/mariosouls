from pathlib import Path
import json,hashlib,configparser
w=Path('work');g=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
p=g/'MarioSouls/install.json';m=json.loads(p.read_text());m['combat']='Asylum-tested experimental independent Mario power, native defenses/HP/stamina, eight move windows, configurable STR/DEX/upgrades and equipment burden; inventory upgrades not implemented'
m['arena_collision_sha256']=hashlib.sha256((g/'MarioSouls/collision/asylum-arena-floor.bin').read_bytes()).hexdigest()
m['source_sha256']={}
for name in ['mariosouls.cpp','mario_native_combat.h','mario_combat_runtime.h','mario_combat_policy.h','mario_arena_floor.h','harness_runtime.h']:
 a=(w/name).read_bytes();b=(g/'MarioSouls/source'/name).read_bytes();assert a==b,name;m['source_sha256'][name]=hashlib.sha256(a).hexdigest()
c=configparser.ConfigParser();c.read(g/'mariosouls.ini');m['combat_settings']=dict(c['Combat'])
p.write_text(json.dumps(m,indent=2));(w/'prototype-install.json').write_text(json.dumps(m,indent=2))
print('Installed source matches; receipt refreshed.')
