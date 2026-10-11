from pathlib import Path
import subprocess,json,hashlib,shutil,configparser
w=Path(__file__).resolve().parents[1]
g=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
r=subprocess.run(['powershell','-NoProfile','-Command','[bool](Get-Process DARKSOULS -ErrorAction SilentlyContinue)'],capture_output=True,text=True,check=True)
assert r.stdout.strip()=='False','Quit game first'
for src,dst in [(w/'build/dinput8.dll',g/'dinput8.dll'),(w/'build/sm64.dll',g/'MarioSouls/sm64.dll')]:shutil.copy2(src,dst)
for name in ['mariosouls.cpp','mario_world_bridge.h','mario_object_combat.h','mario_arena_floor.h','mario_native_combat.h','mario_param_templates.h','mario_combat_policy.h','mario_combat_runtime.h','harness_runtime.h','harness_policy.h','controller_input.h','interaction_state.h']:shutil.copy2(w/name,g/'MarioSouls/source'/name)
for name in ['libsm64.c','libsm64.h','load_surfaces.c','load_surfaces.h','fake_interaction.h']:shutil.copy2(w/'libsm64/src'/name,g/'MarioSouls/source'/name)
shutil.copy2(w/'progression/asylum-arena-floor.bin',g/'MarioSouls/collision/asylum-arena-floor.bin')
p=g/'MarioSouls/install.json';m=json.loads(p.read_text());m.update(version='0.6-native-world-bridge',proxy_sha256=hashlib.sha256((g/'dinput8.dll').read_bytes()).hexdigest(),sm64_sha256=hashlib.sha256((g/'MarioSouls/sm64.dll').read_bytes()).hexdigest(),combat='Mario-specific native attack records, pre-defense physical power, attack windows and native stamina; native world regression in progress',harness='Bounded leased test commands and HP-confirmed telemetry')
p.write_text(json.dumps(m,indent=2));(w/'prototype-install.json').write_text(json.dumps(m,indent=2))
c=configparser.ConfigParser();c.optionxform=str;c.read(g/'mariosouls.ini')
if 'Combat' not in c:c['Combat']={'BasePower':'100','UpgradePower':'10','UpgradeLevel':'0','StatScaling':'0','FixedPower':'100'}
with (g/'mariosouls.ini').open('w') as f:c.write(f)
print('Installed experimental combat build',m['proxy_sha256'])
