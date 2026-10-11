from pathlib import Path
import sys,time,json,subprocess
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
r=Runner(int(sys.argv[1]),w/'interaction-v2'/('baseline-'+time.strftime('%H%M%S')))
try:
 r.native_off(1810960);r.capture('native-before')
 r.drive.hold('0x45',140)
 for i in range(55):
  r.observe(1810960,'item');time.sleep(.08)
  if i in [3,10,20,35]:r.capture('item-'+str(i))
 r.capture('native-after')
finally:save(r.out/'trace.json',r.trace);r.close()
print(r.out)
