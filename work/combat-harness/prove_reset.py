"""Initial bounded in-game proof using existing bridge; no save or health writes."""
import sys,time,json,subprocess,math
from pathlib import Path
from host import Host,WORK,StateError
from geometry import Geometry
from um.win import Drive,shot
pid=int(sys.argv[1]);h=Host(pid,write=True);g=Geometry();d=Drive('DARKSOULS');rows=[]
def observe(label):
 s=h.snapshot();rows.append(dict(event=label,**s));return s
def bridge(*args):
 h.snapshot();subprocess.run([sys.executable,str(WORK/'bridge.py'),*args],check=True,stdout=subprocess.DEVNULL);time.sleep(.3)
def wait_for(pred,timeout,label):
 deadline=time.monotonic()+timeout
 while time.monotonic()<deadline:
  s=observe(label)
  if not s['focus']:raise StateError('Game focus lost')
  if pred(s):return s
  time.sleep(.1)
 raise StateError('Timed out: '+label)
try:
 d.focus();time.sleep(1);bridge('Mode=2')
 p=[-14.35,185.4,-22.2];g.floor(p);h.warp(p,.35)
 time.sleep(1);s=observe('before');assert s['target']['hp']>0
 shot(str(Path(__file__).parent/'hollow-fixture.png'),exe='DARKSOULS.exe',scale=.5)
 bridge('Mode=1')
 for i in range(35):
  s=observe('pre-attack')
  if not s['focus']:raise StateError('Focus lost')
  if s['target']['hp']<=0:break
  pos=s['player']['position'];enemy=s['target']['position'];dx,dz=enemy[0]-pos[0],enemy[2]-pos[2];distance=math.hypot(dx,dz)
  cam=h.chain(0x137d6dc,0,0x3c,0x60);f=h.v(cam+0x30);sx=-f[2]*dx+f[0]*dz;sy=-f[0]*dx-f[2]*dz;dominant=max(abs(sx),abs(sy),.001)
  x=0 if abs(sx)<dominant*.45 else (1 if sx>0 else -1);y=0 if abs(sy)<dominant*.45 else (1 if sy>0 else -1)
  if distance>1:bridge('Frames=4',f'X={x}',f'Y={y}','Buttons=0');time.sleep(.15)
  else:bridge('Frames=4','Buttons=2');time.sleep(.15);bridge('Frames=18','Buttons=0');time.sleep(.65)
  print('attack',i,observe('post-attack')['target']['hp'],flush=True)
 s=observe('after-attacks');assert s['target']['hp']==0,'Bounded attacks did not kill fixture'
 bridge('Mode=2');wait_for(lambda s:s['native_idle'] and not s['interaction'],5,'native-settle')
 h.warp([3.1,196,5.7],math.pi);time.sleep(1)
 for attempt in range(2):
  d.hold('0x51',140)
  try:wait_for(lambda s:s['menu'].get('0x40')==2,5,'bonfire-menu');break
  except StateError:
   if attempt:raise
 time.sleep(3);d.hold('0x0D',140)
 wait_for(lambda s:not s['menu'] and not s['interaction'] and s['native_idle'],10,'bonfire-left')
 h.warp(p,.35)
 s=wait_for(lambda s:s['target'] and s['target']['hp']==44 and not s['menu'] and not s['interaction'] and s['native_idle'],15,'reset-verified')
 print('Reset restored',s['target'],flush=True)
except Exception as e:
 print(type(e).__name__,str(e),flush=True);raise
finally:
 try:bridge('Mode=2','Frames=0','Buttons=0')
 except Exception:pass
 (Path(__file__).parent/'reset-proof.json').write_text(json.dumps(rows,indent=2))
 d.close();h.close()
