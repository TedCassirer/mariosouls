from pathlib import Path
import sys,time,math,subprocess,json
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
from um.win import shot
r=Runner(int(sys.argv[1]),w/'ui-render'/('check-'+time.strftime('%H%M%S')));target=1810960
def capture(name):r.capture(name);print(name,flush=True)
def bridge(mode):subprocess.run([sys.executable,str(w/'bridge.py'),f'Mode={mode}'],check=True)
try:
 r.native_off(target);r.warp([3.1,196,6.55],math.pi,target);r.drive.key('0x4F');time.sleep(.7);bridge(1);time.sleep(1)
 capture('prompt')
 r.drive.hold('0x51',140);time.sleep(2);capture('bonfire-menu')
 s=r.observe(target)
 if s['menu']:
  r.drive.hold('0x0D',140);time.sleep(5)
 capture('after-rest');r.drive.hold('0x20',150);time.sleep(.2);capture('jump');time.sleep(1.5)
 r.drive.hold('0x23',140);time.sleep(.5)
 for i in range(3):r.drive.hold('0x27',140);time.sleep(.3)
 r.drive.hold('0x0D',140);time.sleep(.7);capture('system-menu')
 r.drive.hold('0x08',140);time.sleep(.4);r.drive.hold('0x08',140);time.sleep(2)
 save(r.out/'state.json',r.observe(target))
 root=r.host.u(0x137dc70);begin=r.host.u(root+4);end=r.host.u(root+8);actors=[]
 for address in range(begin+4,end,4):
  try:
   p=r.host.u(address)
   if p:
    a=r.host.actor(p)
    if a['model']=='c0000' or a['team'] not in [6,7]:actors.append(a)
  except Exception:pass
 save(r.out/'actors.json',actors)
finally:r.close()
print(r.out)
