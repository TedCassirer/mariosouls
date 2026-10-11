from pathlib import Path
import sys,time,math,json
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
from transport import Session
from object_state import objects
r=Runner(int(sys.argv[1]));record={};events=[]
try:
 r.native_off(1810810);r.warp([8.4,198.25,-11.85],-math.pi/2,1810810);time.sleep(.7);r.capture('Mario pot before');record['before']=objects(r.host)
 assert next(o for o in record['before'] if o['index']==60)['hp']==1,'Pot fixture is already broken; select an intact fixture'
 r.session=Session(1810810,pid=r.host.pid);r.session.command(1)
 for i in range(5):r.session.command(3,6,0,0,-1);record.setdefault('blocked',[]).append(r.observe(1810810))
 record['before_punch']=objects(r.host);r.capture('Mario pot blocks walking')
 r.session.command(3,8,2);r.session.command(3,45,0);record['after']=objects(r.host);r.capture('Mario pot after')
 for i in range(4):r.session.command(3,5,0,0,-1);record.setdefault('through',[]).append(r.observe(1810810))
 r.capture('Mario crosses broken pot')
 events=r.session.close();r.session=None
finally:
 if r.session:events=r.session.close();r.session=None
 save(r.out/'mario-object.json',record);save(r.out/'setup.json',r.trace);(r.out/'events.jsonl').write_text(''.join(json.dumps(e)+'\n' for e in events))
 try:r.native_off(1810810);r.warp([3.1,196,5.7],math.pi,1810810)
 except Exception as e:print('cleanup',e)
 r.close()
print(r.out)
