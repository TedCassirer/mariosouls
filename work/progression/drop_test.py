from pathlib import Path
import sys,time,math,json
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save,summarize
from transport import Session
pid=int(sys.argv[1]);height=int(sys.argv[2]);r=Runner(pid);result={'drop_test_meters':height};events=[]
try:
 r.reset();r.native_off(1810960);r.warp([3.1,196,5.7],math.pi,1810960)
 r.until(lambda s:s['target'] and s['target']['hp']>0,10,1810960,'bonfire-anchor-loaded');time.sleep(1)
 r.session=Session(1810960,suppress_outgoing=True,pid=pid);r.session.command(1);r.session.command(3,20)
 result['before']=r.host.snapshot(1810960);r.session.command(5,frames=height);deadline=time.monotonic()+9
 while time.monotonic()<deadline:
  r.session.poll()
  if any(e.get('reason')=='player_died' for e in r.session.events):break
  time.sleep(.05)
 events=r.session.close();r.session=None;result.update(summarize(events));result['fall_events']=[e for e in events if e['type']=='environmental_fall_damage']
 if any(e.get('reason')=='player_died' for e in events):result['respawn']=r.wait_respawn(45)
 result['after']=r.host.snapshot(1810960);r.capture('fall-result')
 result['status']='measured'
except Exception as e:result['error']=str(e)
finally:
 if r.session:events=r.session.close();r.session=None
 save(r.out/'drop.json',result);(r.out/'drop-events.jsonl').write_text(''.join(json.dumps(e)+'\n' for e in events));r.close()
 print({k:v for k,v in result.items() if k not in ['before','after','respawn','hits','incoming']});print(r.out)


