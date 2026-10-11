from pathlib import Path
import sys,time
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
r=Runner(int(sys.argv[1]));samples=[]
try:
 s=r.observe(1810810);assert s['interaction'],'Expected attached ladder'
 time.sleep(2);r.capture('Mario ladder pause')
 for i in range(12):
  s=r.observe(1810810);samples.append(s)
  if not s['interaction']:break
  r.drive.hold('0x53',750);time.sleep(.1)
 time.sleep(2);samples.append(r.observe(1810810));r.capture('Mario ladder dismount')
 r.drive.hold('0x20',100);time.sleep(.25);samples.append(r.observe(1810810));time.sleep(1.5)
finally:save(r.out/'ladder.json',samples);r.close()
print(r.out);print([(s['player']['position'],s['player']['tracks'],s['player']['flags2']) for s in samples])
