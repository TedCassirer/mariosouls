from pathlib import Path
import sys,time,math,subprocess
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
r=Runner(int(sys.argv[1]),w/'interaction-v2'/('door-'+time.strftime('%H%M%S')));t=1810960
def bridge(mode):subprocess.run([sys.executable,str(w/'bridge.py'),f'Mode={mode}'],check=True);time.sleep(.6)
try:
 r.native_off(t);r.warp([14.99,208.5,-20.55],0,t);r.drive.key('0x4F');bridge(1);r.capture('before')
 r.drive.hold('0x57',130);time.sleep(.4);r.capture('prompt');r.drive.hold('0x51',140);time.sleep(.7);s=r.observe(t);print('Mario door',s['menu'],s['player']['position']);r.capture('response')
 if s['menu']:r.drive.hold('0x0D',140);time.sleep(.8)
 else:
  r.native_off(t);r.drive.hold('0x57',250);time.sleep(.5);r.drive.hold('0x51',140);time.sleep(.7);s=r.observe(t);print('Native door',s['menu']);r.capture('native-response')
  if s['menu']:r.drive.hold('0x0D',140);time.sleep(.8)
 r.native_off(t);r.warp([3.1,196,6.55],math.pi,t);r.drive.key('0x4F');bridge(1);r.drive.hold('0x51',140);time.sleep(3);s=r.observe(t);r.capture('rest')
 if s['menu']:r.drive.hold('0x0D',140);time.sleep(4)
 print('final HP',r.observe(t)['player']['hp'])
finally:save(r.out/'trace.json',r.trace);r.close();print(r.out)
