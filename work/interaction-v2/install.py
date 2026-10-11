from pathlib import Path
import subprocess,sys,shutil,json,hashlib,configparser
w=Path(__file__).resolve().parents[1]
g=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
r=subprocess.run(['powershell','-NoProfile','-Command','[bool](Get-Process DARKSOULS -ErrorAction SilentlyContinue)'],capture_output=True,text=True,check=True)
assert r.stdout.strip()=='False','Close game normally first'
for src,dst in [(w/'build/dinput8.dll',g/'dinput8.dll'),(w/'build/sm64.dll',g/'MarioSouls/sm64.dll')]:shutil.copy2(src,dst)
names=["mario_action_effects.h","mario_flask.h",'mariosouls.cpp','mario_action_input.h','mario_action_runtime.h','mario_ui_order.h','build_plugin.py','build_sm64.py','sm64_smoke.c']
for name in names:shutil.copy2(w/name,g/'MarioSouls/source'/name)
for name in ['libsm64.c','libsm64.h','host_pose.h']:shutil.copy2(w/'libsm64/src'/name,g/'MarioSouls/source'/name)
shutil.copy2(w/'libsm64/src/decomp/game/rendering_graph_node.c',g/'MarioSouls/source/rendering_graph_node.c')
p=g/'MarioSouls/install.json';m=json.loads(p.read_text());m['version']='0.7-action-integration-candidate'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
m['proxy_sha256']=sha(g/'dinput8.dll');m['sm64_sha256']=sha(g/'MarioSouls/sm64.dll')
p.write_text(json.dumps(m,indent=2));(w/'prototype-install.json').write_text(json.dumps(m,indent=2))
c=configparser.ConfigParser();c.optionxform=str;c.read(g/'mariosouls.ini');c['Test'].update(Mode='0',Frames='0',Buttons='0',X='0',Y='0');c['Harness'].update(Armed='0',Session='0')
with (g/'mariosouls.ini').open('w') as f:c.write(f)
print('Installed candidate',m['proxy_sha256'],m['sm64_sha256'])
