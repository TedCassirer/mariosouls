from pathlib import Path
import sys,time,math,subprocess
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
from transport import Session
from um.win import Recorder
r=Runner(int(sys.argv[1]),w/'interaction-v2'/('final-'+time.strftime('%H%M%S')));anchor=1810960;rec=None;events=[]
def step(label,seconds):
 end=time.monotonic()+seconds
 while time.monotonic()<end:r.observe(anchor,label);time.sleep(.05)
def bridge(mode):subprocess.run([sys.executable,str(w/'bridge.py'),f'Mode={mode}'],check=True);step('mode',.6)
def setup(p,y):r.native_off(anchor);r.warp(p,y,anchor);r.drive.key('0x4F');bridge(1)
def key(k,ms=140):r.observe(anchor,'input');r.drive.hold(k,ms)
try:
 setup([-14.35,185.4,-22.2],.35);s=r.host.snapshot(1810101);assert s['target']['hp']==44
 r.session=Session(1810101,pid=r.host.pid);r.session.command(3,4,2);r.session.command(3,45,0);events=r.session.close();r.session=None
 s=r.host.snapshot(1810101);assert s['target']['hp']<44;print('Punch damage',44-s['target']['hp']);r.capture('punch')
 setup([-7.76,208.55,-1],0);before=r.observe(anchor)['player']['hp'];deadline=time.monotonic()+18
 while time.monotonic()<deadline:
  s=r.observe(anchor,'incoming')
  if s['player']['hp']<before:break
  time.sleep(.05)
 assert s['player']['hp']<before and s['player']['hp']>0,'No ordinary enemy hit observed'
 damaged=s['player']['hp'];print('Incoming native damage',before-damaged)
 setup([3.1,196,5.7],0);key('0x4A',1100);step('camera',.4);r.capture('before-drink')
 rec=Recorder(exe='DARKSOULS.exe',out=str(r.out/'proof'),pid=r.host.pid);rec.start();step('record-idle',1)
 key('0x45',700);step('drink',3);s=r.observe(anchor,'healed');assert s['player']['hp']==min(damaged+300,s['player']['hp_max']);print('Estus healed',s['player']['hp']-damaged);r.capture('after-drink')
 key('0x57',300);step('move',.3);key('0x20');step('jump',1.5)
 rec.stop();rec=None
 setup([3.1,196,6.55],math.pi);key('0x53',700);step('away',.8);assert r.host.u(r.host.u(0x13786d0)+0x58)==0,'Action prompt did not clear after walking away'
 a=r.observe(anchor)['player']['position'];key('0x51');key('0x41',350);step('no-prompt-move',.3);b=r.observe(anchor)['player']['position'];assert math.dist(a,b)>.3 and not r.observe(anchor)['interaction'];print('Rejected interaction retained movement')
 key('0x23');step('system-menu',.8);assert r.observe(anchor)['menu'];r.capture('system-menu');key('0x23');step('system-close',.8);assert not r.observe(anchor)['menu'];print('System menu resumed Mario')
 setup([3.1,196,6.55],math.pi);key('0x51');step('rest',3);assert r.observe(anchor)['menu'];r.capture('rest');key('0x0D');step('leave',4)
 y=r.observe(anchor)['player']['position'][1];key('0x20');step('after-rest-jump',.25);assert r.observe(anchor)['player']['position'][1]>y+.3;step('land',1.5)
 s=r.observe(anchor,'final');assert s['player']['hp']==s['player']['hp_max'] and not s['menu'] and not s['interaction'];r.capture('final')
 save(r.out/'final-state.json',s);print('PASS: combat, incoming damage, direct Estus, rejected request, movement, menus, bonfire and jump')
finally:
 if rec:rec.stop()
 save(r.out/'trace.json',r.trace);save(r.out/'combat-events.json',events);r.close();print(r.out)
