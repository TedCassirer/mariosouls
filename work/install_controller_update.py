from pathlib import Path
import hashlib,json,shutil,subprocess
w=Path(__file__).resolve().parent
game=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
running=subprocess.run(['powershell','-NoProfile','-Command','[bool](Get-Process -Name DARKSOULS -ErrorAction SilentlyContinue)'],capture_output=True,text=True,check=True)
assert running.stdout.strip()=='False','Quit the game before replacing the loaded plugin.'
shutil.copy2(w/'build/dinput8.dll',game/'dinput8.dll')
for name in ['mariosouls.cpp','controller_input.h','controller_test.cpp','build_plugin.py']:
 shutil.copy2(w/name,game/'MarioSouls/source'/name)
manifest_path=game/'MarioSouls/install.json'
manifest=json.loads(manifest_path.read_text())
manifest.update(version='0.2-experimental-controller',proxy_sha256=hashlib.sha256((game/'dinput8.dll').read_bytes()).hexdigest(),controller='XInput: left stick, A, B, LT; hold L3+R3 600 ms to toggle; Start returns to native menus')
manifest_path.write_text(json.dumps(manifest,indent=2))
(w/'prototype-install.json').write_text(json.dumps(manifest,indent=2))
notes=w.parent/'outputs/MarioSouls controls and notes.txt'
text=notes.read_text(encoding='utf-8').replace('local build 0.1','local build 0.2')
text=text.replace('CONTROLS\n','KEYBOARD AND MOUSE\n',1)
text=text.replace('Switch back with F6 for ladders, doors, bonfires, dialogue, items, and menus.', '''CONTROLLER (XBOX BUTTON POSITIONS)
Hold both stick clicks (L3 + R3) together for about one second: switch Mario on/off. Release before toggling again. F6 still works.
Left stick: analog walking/running relative to the camera.
Right stick: normal Dark Souls camera.
A (bottom face button): jump.
B (right face button): punch, kick, or dive.
LT: crouch; press in the air to ground pound.
Run, hold LT, then press A: long jump. Stand still, hold LT, then press A: backflip.
Start: return to native controls and open the Dark Souls menu.
The controller is detected automatically and reconnects after unplugging. Keyboard and mouse remain available. Slight stick drift is ignored. Mario's movement, A/B and LT are withheld from the native game while Mario is active so jumping does not also interact with a ladder.

Switch back with F6 or the stick-click shortcut for ladders, doors, bonfires, dialogue, items, and menus.''')
notes.write_text(text,encoding='utf-8')
print('Controller update installed; restore manifest refreshed.')
