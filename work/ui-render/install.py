from pathlib import Path
import subprocess,shutil,json,hashlib,configparser,sys
w=Path(__file__).resolve().parents[1]
g=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
r=subprocess.run(['powershell','-NoProfile','-Command','[bool](Get-Process DARKSOULS -ErrorAction SilentlyContinue)'],capture_output=True,text=True,check=True)
assert r.stdout.strip()=='False','Game must be closed'
shutil.copy2(w/'build/dinput8.dll',g/'dinput8.dll')
for name in ['mariosouls.cpp','mario_ui_order.h']:shutil.copy2(w/name,g/'MarioSouls/source'/name)
p=g/'MarioSouls/install.json';m=json.loads(p.read_text());m['proxy_sha256']=hashlib.sha256((g/'dinput8.dll').read_bytes()).hexdigest()
m['render']='UI boundary work in progress' if '--trace' in sys.argv else 'Mario composited before native UI'
p.write_text(json.dumps(m,indent=2));(w/'prototype-install.json').write_text(json.dumps(m,indent=2))
c=configparser.ConfigParser();c.optionxform=str;c.read(g/'mariosouls.ini');c['MarioSouls']['UITrace']='1' if '--trace' in sys.argv else '0'
c['Test'].update(Mode='0',Frames='0',Buttons='0',X='0',Y='0')
with (g/'mariosouls.ini').open('w') as f:c.write(f)
print('Installed',m['proxy_sha256'])
