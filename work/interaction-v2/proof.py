from pathlib import Path
import sys,time,math,subprocess
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
from um.win import Recorder
r=Runner(int(sys.argv[1]),w/'interaction-v2'/('proof-'+time.strftime('%H%M%S')));target=1810960;rec=None
def step(label,seconds):
 end=time.monotonic()+seconds
 while time.monotonic()<end:r.observe(target,label);time.sleep(.05)
def key(k,ms=140):r.observe(target,'before-input');r.drive.hold(k,ms)
def setup(p,y):
 r.native_off(target);r.warp(p,y,target);r.drive.key('0x4F');time.sleep(.5)
 subprocess.run([sys.executable,str(w/'bridge.py'),'Mode=1'],check=True);step('ready',.8)
def start(name):
 global rec
 rec=Recorder(exe='DARKSOULS.exe',out=str(r.out/name),pid=r.host.pid);rec.start();step('record-ready',1)
try:
 setup([3.1,196,5.7],0);key('0x4A',1100);step('camera',.5);start('estus')
 key('0x45');step('raise',.4);r.capture('drink');step('drink',3);key('0x57',300);step('move',.25);key('0x20');step('jump-land',2);rec.stop();rec=None
 setup([3.1,196,6.55],math.pi);start('bonfire');key('0x51');step('sit',3);assert r.observe(target)['menu'];r.capture('rest');key('0x0D');step('stand',3.5)
 assert not r.observe(target)['menu'] and not r.observe(target)['interaction'];y=r.observe(target)['player']['position'][1];key('0x20');step('jump',.3);assert r.observe(target)['player']['position'][1]>y+.3;step('land',2);rec.stop();rec=None
 s=r.observe(target,'final');assert s['player']['hp']==s['player']['hp_max'];assert s['stats']['souls']==184;assert not s['nearby'];r.capture('final');save(r.out/'final-state.json',s)
 print('PASS: installed final build Estus, rest, leave, immediate jump; full HP and recovered souls')
finally:
 if rec:rec.stop()
 save(r.out/'trace.json',r.trace);r.close();print(r.out)
