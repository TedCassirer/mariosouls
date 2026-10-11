from pathlib import Path
import sys,time,math,json
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save,summarize
from transport import Session
pid=int(sys.argv[1]);r=Runner(pid);results=[]
recipes={
 'combo':[(3,2,0,0),(5,0,0,0),(3,2,0,0),(5,0,0,0),(3,2,0,0),(40,0,0,0)],
 'jump_kick':[(3,1,0,0),(6,2,0,0),(50,0,0,0)],
 'ground_pound':[(12,1,0,0),(8,4,0,0),(50,0,0,0)],
 'dive':[(41,2,0,-1,35),(40,0,0,0)],
 'slide_kick':[(15,0,0,-1),(3,4,0,-1),(6,6,0,-1),(40,0,0,0)],
 'navigation':[(8,1,0,0),(50,0,0,0)],
 'exhaustion':[(3,2,0,0),(4,0,0,0)]*30,
}
try:
 for name,steps in recipes.items():
  if len(sys.argv)>2 and name not in sys.argv[2:]:continue
  result={'name':name};events=[]
  try:
   r.reset();r.native_off(1810960);r.warp([3.1,196,5.7],math.pi if name in ['dive','slide_kick'] else 0,1810960)
   r.until(lambda s:s['target'] and s['target']['hp']>0,10,1810960,'anchor-loaded');time.sleep(.5)
   r.session=Session(1810960,pid=pid);r.session.command(1);r.session.command(3,20)
   for step in steps:r.session.command(3,*step)
   events=r.session.close();r.session=None;result.update(summarize(events));result['status']='measured'
  except Exception as e:result['error']=str(e)
  finally:
   if r.session:events=r.session.close();r.session=None
  (r.out/(name+'-events.jsonl')).write_text(''.join(json.dumps(e)+'\n' for e in events))
  results.append(result);save(r.out/'moves.json',results);print(name,result.get('stamina_charged'),[(e['move'],e['cost'],e['paid']) for e in result.get('attacks',[])],result.get('error'),flush=True)
finally:r.close();print(r.out)
