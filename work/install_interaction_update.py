from pathlib import Path
import hashlib,json,shutil,subprocess
w=Path(__file__).resolve().parent
game=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
r=subprocess.run(['powershell','-NoProfile','-Command','[bool](Get-Process -Name DARKSOULS -ErrorAction SilentlyContinue)'],capture_output=True,text=True,check=True)
assert r.stdout.strip()=='False','Quit the game before replacing loaded libraries.'
backup=w/'interaction-backup'
for i,p in enumerate(Path(r'C:\Users\Ted\Documents\NBGI\DarkSouls').rglob('*.sl2')):
 target=backup/f'pre-interaction-save-{i}.sl2'
 if not target.exists():shutil.copy2(p,target)
shutil.copy2(w/'build/dinput8.dll',game/'dinput8.dll')
shutil.copy2(w/'build/sm64.dll',game/'MarioSouls/sm64.dll')
for name in ['mariosouls.cpp','controller_input.h','controller_test.cpp','interaction_state.h','interaction_test.cpp','build_plugin.py','build_sm64.py']:
 if (w/name).exists():shutil.copy2(w/name,game/'MarioSouls/source'/name)
dest=game/'MarioSouls/source/libsm64-pose';dest.mkdir(exist_ok=True)
for name in ['libsm64.c','libsm64.h']:shutil.copy2(w/'libsm64/src'/name,dest/name)
manifest_path=game/'MarioSouls/install.json'
manifest=json.loads(manifest_path.read_text())
manifest.update(version='0.3-experimental-interactions',proxy_sha256=hashlib.sha256((game/'dinput8.dll').read_bytes()).hexdigest(),sm64_sha256=hashlib.sha256((game/'MarioSouls/sm64.dll').read_bytes()).hexdigest(),controller='XInput: left stick, A, B, LT; hold L3+R3 600 ms to toggle; Y interacts; Start opens native menu with Mario visible',interactions='Q / Y hands movement to native game with Mario pose rendering; resumes Mario after grounded native idle')
manifest_path.write_text(json.dumps(manifest,indent=2))
(w/'prototype-install.json').write_text(json.dumps(manifest,indent=2))
print('Both interaction libraries installed; previous libraries and saves backed up; restore manifest refreshed.')
