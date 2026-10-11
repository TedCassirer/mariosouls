from host import GAME,WORK
from pathlib import Path
import hashlib,shutil,json,subprocess
r=subprocess.run(['powershell','-NoProfile','-Command','[bool](Get-Process -Name DARKSOULS -ErrorAction SilentlyContinue)'],capture_output=True,text=True,check=True)
assert r.stdout.strip()=='False','Quit the game before replacing the plugin'
shutil.copy2(WORK/'build/dinput8.dll',GAME/'dinput8.dll')
for name in ['mariosouls.cpp','harness_runtime.h','harness_policy.h','controller_input.h','interaction_state.h']:
 shutil.copy2(WORK/name,GAME/'MarioSouls/source'/name)
p=GAME/'MarioSouls/install.json';m=json.loads(p.read_text());m.update(version='0.4-experimental-combat-harness',proxy_sha256=hashlib.sha256((GAME/'dinput8.dll').read_bytes()).hexdigest(),harness='Opt-in leased frame commands and HP-confirmed telemetry; normal interaction build preserved; no balance changes')
p.write_text(json.dumps(m,indent=2));(WORK/'prototype-install.json').write_text(json.dumps(m,indent=2))
print('Instrumented plugin installed; restore receipt refreshed.')
