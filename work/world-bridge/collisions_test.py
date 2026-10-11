from pathlib import Path
import sys,time,math,json,subprocess
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
from transport import Session
r=Runner(int(sys.argv[1]));mode=sys.argv[2] if len(sys.argv)>2 else 'door';events=[];record={}
fixtures={'door':([-14,185.5,-31.65],math.pi/2,1810101),'enemy':([-14.67,185.5,-21.4],0,1810101),'wall':([-9.3,201.9,14.8],math.pi,1810101),'gate':([-11,196.2,-.4],-math.pi/2,1810960)}
fixtures['main-door']=([3.2,198.2,-1.8],0,1810810)
p,heading,target=fixtures[mode]
try:
 r.reset();r.native_off(target);r.warp(p,heading,target);time.sleep(1)
 r.until(lambda s:s['target'] and s['target']['hp']>0,15,target,'target-loaded');r.capture(mode+' before');record['before']=r.observe(target)
 r.session=Session(target,suppress_outgoing=True,pid=r.host.pid);r.session.command(1);time.sleep(.3)
 for i in range(6):r.session.command(3,10,0,0,-1);record.setdefault('samples',[]).append(r.observe(target))
 r.session.command(3,15,0);record['after']=r.observe(target);r.capture(mode+' collision')
 events=r.session.close();r.session=None
finally:
 if r.session:events=r.session.close();r.session=None
 save(r.out/'world-collision.json',record);save(r.out/'setup.json',r.trace);(r.out/'events.jsonl').write_text(''.join(json.dumps(e)+'\n' for e in events));r.close()
print(r.out);print('positions',*[s['player']['position'] for s in record.get('samples',[])])
