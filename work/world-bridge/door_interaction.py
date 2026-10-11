from pathlib import Path
import sys,time,math,json,subprocess
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
r=Runner(int(sys.argv[1]));records=[]
try:
 r.native_off(1810810);r.warp([3.2,198.2,-3.35],0,1810810);time.sleep(.5)
 subprocess.run([sys.executable,str(w/'bridge.py'),'Mode=1'],check=True);time.sleep(1)
 r.capture('Mario closed door');r.drive.hold('0x51',140)
 for i in range(35):records.append(r.observe(1810810));time.sleep(.2)
 r.capture('Mario door interaction')
finally:save(r.out/'door-interaction.json',records);save(r.out/'setup.json',r.trace);r.close()
print(r.out);print('states',[(s['menu'],s['player']['tracks']) for s in records[::5]])
