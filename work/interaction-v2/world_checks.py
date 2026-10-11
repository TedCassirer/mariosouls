from pathlib import Path
import sys,time,math,subprocess,json
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
r=Runner(int(sys.argv[1]),w/'interaction-v2'/('world-'+time.strftime('%H%M%S')));target=1810960
def step(label,seconds):
 end=time.monotonic()+seconds
 while time.monotonic()<end:r.observe(target,label);time.sleep(.05)
def setup(p,y):
 r.native_off(target);r.warp(p,y,target);r.drive.key('0x4F');time.sleep(.4)
 subprocess.run([sys.executable,str(w/'bridge.py'),'Mode=1'],check=True);step('ready',.8)
def key(k,ms=140):r.observe(target,'before-input');r.drive.hold(k,ms)
try:
 setup([0,196.09,21.7],2.3562);r.capture('ladder-before');key('0x51');step('ladder-mount',2.4)
 s=r.observe(target);assert s['interaction'],'Ladder did not attach';r.capture('ladder-mounted')
 key('0x53',1800);step('ladder-pause',.7);a=r.observe(target)['player']['position'];key('0x4A',900);r.capture('ladder-pause');step('ladder-stationary',.7);b=r.observe(target)['player']['position'];assert math.dist(a,b)<.05
 key('0x57',650);step('ladder-up',.3);assert r.observe(target)['player']['position'][1]>b[1]+.3
 for j in range(14):
  if not r.observe(target)['interaction']:break
  key('0x53',700);step('ladder-down',.1)
 step('ladder-exit',1);assert not r.observe(target)['interaction'];r.capture('ladder-dismount')
 y=r.observe(target)['player']['position'][1];key('0x20');step('ladder-jump',.25);assert r.observe(target)['player']['position'][1]>y+.3;r.capture('ladder-jump');step('land',1.5)
 setup([-8.595,208.5,-9.6],math.pi);key('0x57',200);step('door-approach',.5);r.capture('locked-before');key('0x51');step('locked-dialog',1);r.capture('locked-dialog');assert r.observe(target)['menu']
 key('0x0D');step('locked-close',1.5);assert not r.observe(target)['menu'];key('0x53',200);step('locked-move',.4)
 setup([3.1,196,5.7],0);key('0x53',400);step('no-prompt-ready',.8)
 h=r.host;menu=h.u(0x13786d0);assert h.u(menu+0x58)==0,'Still near an action prompt'
 a=r.observe(target)['player']['position'];key('0x51');key('0x57',350);step('rejected-request-move',.2);b=r.observe(target)['player']['position'];assert math.dist(a,b)>.3;assert not r.observe(target)['interaction'];r.capture('no-prompt-movement')
 key('0x23');step('system-menu',.6);r.capture('system-menu');assert r.observe(target)['menu'];key('0x23');step('system-close',1);assert not r.observe(target)['menu']
 setup([3.1,196,6.55],math.pi);key('0x51');step('final-rest',3);assert r.observe(target)['menu'];r.capture('final-rest');key('0x0D');step('final-leave',4);assert not r.observe(target)['interaction']
 r.capture('final-ready');print('PASS: ladder down/pause/up/dismount/jump; locked door; rejected request movement; system menu; bonfire')
finally:save(r.out/'trace.json',r.trace);r.close();print(r.out)
