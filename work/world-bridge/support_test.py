from pathlib import Path
import sys,time,subprocess,math
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
from contacts import contacts
r=Runner(int(sys.argv[1]));samples=[]
try:
 r.native_off(1810810)
 r.geo.floor([3.1,196,7.44])
 r.host.warp([3.1,198,7.44],math.pi,1810810);time.sleep(1)
 samples.append(dict(native=r.observe(1810810),collision=contacts(r.host)))
 subprocess.run([sys.executable,str(w/'bridge.py'),'Mode=1'],check=True)
 for i in range(45):samples.append(dict(native=r.observe(1810810),collision=contacts(r.host)));time.sleep(.1)
 r.capture('Mario native support')
finally:save(r.out/'support.json',samples);save(r.out/'setup.json',r.trace);r.close()
print(r.out);print([s['collision'] for s in samples[::10]])
