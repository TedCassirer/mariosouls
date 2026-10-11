from pathlib import Path
import sys,time,subprocess,math
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
r=Runner(int(sys.argv[1]),w/'interaction-v2'/('check-'+time.strftime('%H%M%S')));target=1810960
def observe(label,seconds):
 end=time.monotonic()+seconds
 while time.monotonic()<end:r.observe(target,label);time.sleep(.05)
def bridge(mode):subprocess.run([sys.executable,str(w/'bridge.py'),f'Mode={mode}'],check=True)
try:
 r.native_off(target);r.warp([3.1,196,5.7],0,target);r.drive.key('0x4F');time.sleep(.5);bridge(1);time.sleep(.8)
 r.drive.hold('0x4A',1100);time.sleep(.4);r.capture('before');r.observe(target,'before')
 r.drive.hold('0x45',140);observe('item-raise',.35);r.capture('item-raise');observe('item-drink',.15);r.capture('item-drink');observe('item-end',2);r.capture('after-item')
 r.drive.hold('0x57',300);observe('move',.2);r.capture('move-after');r.drive.hold('0x20',150);observe('jump',.3);r.capture('jump-after');observe('land',1.5)
 r.native_off(target);r.warp([3.1,196,6.55],math.pi,target);r.drive.key('0x4F');time.sleep(.5);bridge(1);time.sleep(.8)
 r.capture('prompt');r.drive.hold('0x51',140);observe('rest',3);r.capture('rest')
 if r.observe(target)['menu']:
  r.drive.hold('0x0D',140);observe('leave',4)
 r.capture('after-rest');r.drive.hold('0x20',150);observe('jump-rest',.3);r.capture('jump-rest');observe('finish',1.5)
finally:save(r.out/'trace.json',r.trace);r.close()
print(r.out)
