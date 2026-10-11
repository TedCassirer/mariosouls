"""Bounded existing-encounter checks. No spawning, save restoration or enemy writes."""
from pathlib import Path
import sys,time,json,math,argparse
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save,summarize
from transport import Session
from host import StateError
p=argparse.ArgumentParser();p.add_argument('--pid',type=int,required=True);p.add_argument('--kind',choices=['projectile','armored','boss'],required=True);p.add_argument('--attack',action='store_true');p.add_argument('--hits',type=int,default=1);p.add_argument('--jump',action='store_true');p.add_argument('--combo',action='store_true');a=p.parse_args();assert 1<=a.hits<=8
fixtures={'projectile':(1810112,[39,194.46,3.14],math.pi),'armored':(1810113,[15.58,208.585,-27.14],math.pi),'boss':(1810800,[3,198.2,-12],0)}
target,pos,heading=fixtures[a.kind];r=Runner(a.pid);events=[];result={'kind':a.kind,'attack':a.attack,'status':'inconclusive'}
try:
 r.reset();r.warp(pos,heading,target)
 before=r.until(lambda s:s['target'] and s['target']['hp']>0,12,target,'target-reacquired');time.sleep(.5);result['before']=before;r.protections(before)
 if not a.attack:r.capture('encounter-start')
 r.session=Session(target,suppress_outgoing=not a.attack,pid=a.pid);r.session.command(1)
 deadline=time.monotonic()+25;lastsample=0
 while time.monotonic()<deadline:
  r.session.poll()
  if any(e.get('reason')=='player_died' for e in r.session.events):break
  if any(e['type']=='confirmed_player_hp_loss' for e in r.session.events) and not a.attack:result['status']='incoming_damage_observed';r.capture('incoming-damage');break
  if a.jump:r.session.command(3,6,1);r.session.command(3,12,0)
  if time.monotonic()-lastsample>.1:r.observe(target,'encounter-observation');lastsample=time.monotonic()
  if a.attack:
   s=r.host.snapshot(target);n=s['target'];h=s['player']
   if not n or n['hp']<=0:break
   dx=n['position'][0]-h['position'][0];dz=n['position'][2]-h['position'][2];distance=math.hypot(dx,dz)
   fx=-math.sin(heading);fz=-math.cos(heading);sx=-fz*dx+fx*dz;sy=-fx*dx-fz*dz
   if distance>=1.3:r.session.command(3,12,0,sx/max(distance,.001),sy/max(distance,.001))
   elif a.combo:
    for f,b in [(3,2),(5,0),(3,2),(5,0),(3,2),(30,0)]:r.session.command(3,f,b)
   else:r.session.command(3,5,2 if distance<1.3 else 0,sx/max(distance,.001),sy/max(distance,.001));r.session.command(3,3,0)
   if sum(e['type']=='confirmed_target_hp_loss' for e in r.session.events)>=a.hits:result['status']='outgoing_damage_observed';r.capture('outgoing-damage');break
  time.sleep(.02)
 events=r.session.close();r.session=None;result.update(summarize(events));result['after']=r.host.snapshot(target)
 if not a.attack and result['damage_to_player']>0:result['status']='incoming_damage_observed'
except Exception as e:result['error']=str(e);r.capture('probe-failure')
finally:
 if r.session:events=r.session.close();r.session=None
 try:
  s=r.host.snapshot(target)
  if s['player']['hp']<=0:r.wait_respawn(45)
  r.native_off(target);r.warp([3.1,196,5.7],math.pi,target)
 except Exception as e:result['cleanup_error']=str(e)
 save(r.out/'probe.json',result);save(r.out/'probe-trace.json',r.trace)
 (r.out/'probe-events.jsonl').write_text(''.join(json.dumps(e)+'\n' for e in events))
 r.close();print({k:v for k,v in result.items() if k not in ['before','after','hits','incoming']});print(r.out)
