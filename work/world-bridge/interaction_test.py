from pathlib import Path
import sys,time,math,subprocess
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
r=Runner(int(sys.argv[1]));kind=sys.argv[2];records=[]
fixtures={'bonfire':([3.1,196,6.55],math.pi),'locked':([-8.595,208.5,-9.6],math.pi),'ladder':([0,196.09,21.7],2.3562)}
try:
 p,y=fixtures[kind];r.native_off(1810810);r.warp(p,y,1810810);time.sleep(1)
 subprocess.run([sys.executable,str(w/'bridge.py'),'Mode=1'],check=True);time.sleep(.8)
 r.capture(kind+' before');r.drive.hold('0x51',140)
 for i in range(25):records.append(r.observe(1810810));time.sleep(.2)
 r.capture(kind+' active')
 if records[-1]['menu']:
  r.drive.hold('0x0D',140)
  for i in range(25):records.append(r.observe(1810810));time.sleep(.2)
 r.capture(kind+' after')
finally:save(r.out/'interaction.json',records);save(r.out/'setup.json',r.trace);r.close()
print(r.out);print('states',[(s['menu'],s['player']['tracks']) for s in records[::5]])
