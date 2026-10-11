from pathlib import Path
import sys,time,math,json,subprocess,argparse
W=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(W/'combat-harness'))
sys.path.insert(0,str(W/'world-bridge'))
from runner import Runner,save
from transport import Session
from um.win import Recorder,shot
from object_state import objects
p=argparse.ArgumentParser();p.add_argument('scene');p.add_argument('--pid',type=int,default=40068);a=p.parse_args()
folder=Path(__file__).parent
name=a.scene+'-'+time.strftime('%H%M%S')
r=Runner(a.pid,folder/(name+'-data'));rec=None;marks=[]
def mark(s):
 marks.append({'label':s,'wall':time.time(),'video_elapsed':time.time()-rec.t_video if rec else None});print(s,flush=True)
def bridge(mode):
 subprocess.run([sys.executable,str(W/'bridge.py'),f'Mode={mode}'],check=True,stdout=subprocess.DEVNULL)
def start():
 global rec
 rec=Recorder(exe='DARKSOULS.exe',out=str(folder/name),max_width=1920,audio=True,cq=18).start();time.sleep(.8);mark('recording')
def activate(target):
 r.session=Session(target,pid=a.pid);r.session.command(1);r.session.command(3,10)
def steps(recipe):
 for step in recipe:r.session.command(3,*step)
try:
 s=r.observe(1810960)
 if s['player']['hp']<=0:r.wait_respawn(45)
 if a.scene in ('moves','wide'):
  t=1810960;r.native_off(t);r.warp([0,196.2,4] if a.scene=='wide' else [-3,196.2,15],math.pi if a.scene=='wide' else 0,t);time.sleep(1)
  if a.scene=='wide':r.drive.key('0x4F');time.sleep(.8)
  activate(t)
  start()
  mark('jump kick');steps([(3,1),(6,2),(50,0)])
  mark('backflip');steps([(8,4),(4,5),(65,0)])
  mark('ground pound');steps([(10,1),(8,4),(45,0)])
  mark('combo');steps([(3,2),(5,0),(3,2),(5,0),(3,2),(45,0)])
  mark('long jump');steps([(10,0,0,-1),(2,4,0,-1),(4,5,0,-1),(45,0)])
  time.sleep(1)
 elif a.scene in ('stomp','ground_pound','combo'):
  r.reset(test=a.scene);activate(1810101);start();mark(a.scene)
  from runner import RECIPES
  steps(RECIPES[a.scene]);time.sleep(2)
 elif a.scene=='bonfire':
  t=1810960;r.native_off(t);r.warp([3.1,196,6.55],math.pi,t);bridge(1);time.sleep(.8);start();time.sleep(.5)
  mark('rest');r.drive.hold('0x51',140);time.sleep(3)
  s=r.observe(t)
  if s['menu']:mark('leave');r.drive.hold('0x0D',140)
  time.sleep(5)
 elif a.scene=='ladder':
  t=1810960;r.native_off(t);r.warp([0,196.09,21.7],2.3562,t);bridge(1);time.sleep(.8);start()
  mark('mount');r.drive.hold('0x51',140);time.sleep(2)
  for i in range(6):r.observe(t);r.drive.hold('0x53',750)
  time.sleep(3)
 elif a.scene=='boss':
  t=1810800;r.reset();r.warp([3,198.2,-12],0,t);r.until(lambda s:s['target'] and s['target']['hp']>0,12,t,'boss');time.sleep(.5);activate(t);start();mark('boss combat')
  end=time.monotonic()+25
  while time.monotonic()<end:
   s=r.observe(t);h=s['player'];n=s['target']
   if not n or n['hp']<=0 or h['hp']<=0:break
   dx=n['position'][0]-h['position'][0];dz=n['position'][2]-h['position'][2];dist=math.hypot(dx,dz)
   if dist>=1.3:steps([(12,0,dx/max(dist,.001),dz/max(dist,.001))])
   else:steps([(3,2),(5,0),(3,2),(5,0),(3,2),(30,0)])
  time.sleep(2)
 elif a.scene=='pots':
  t=1810810;r.native_off(t)
  obj=next(o for o in objects(r.host) if o['index']==61 and o['hp']==1)
  print(obj,flush=True);r.warp([10.3,198.2,-12.50],-math.pi/2,t);time.sleep(.8);activate(t);start()
  mark('approach');steps([(7,0,0,-1)]);mark('punch');steps([(8,2),(35,0)]);mark('continue');steps([(20,0,0,-1),(25,0)])
  time.sleep(1)
 else:raise ValueError(a.scene)
except Exception as e:
 mark('error: '+str(e));print(type(e).__name__,str(e),flush=True)
finally:
 if rec:rec.stop()
 if r.session:save(r.out/'events.json',r.session.close());r.session=None
 save(r.out/'marks.json',marks);save(r.out/'trace.json',r.trace)
 r.close()
print('TAKE',folder/name,flush=True)
