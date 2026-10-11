from pathlib import Path
import sys,time,math,json,subprocess
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from host import Host
from transport import Session
from um.win import Drive,shot
h=Host(int(sys.argv[1]),write=True);d=Drive('DARKSOULS');assert d.focus();s=h.snapshot(1810960)
assert s['player']['name']=='test' and s['player']['hp']>0
subprocess.run([sys.executable,str(w/'bridge.py'),'Mode=2'],check=True);time.sleep(.8)
h.warp([3.1,196,5.7],math.pi,1810960);time.sleep(1)
records=[];session=Session(1810960,suppress_outgoing=True,pid=h.pid)
try:
 session.command(1);time.sleep(.5);records.append(h.snapshot(1810960))
 session.command(3,45,0,0,-1);records.append(h.snapshot(1810960))
 session.command(3,15,0);records.append(h.snapshot(1810960))
 session.command(3,30,0,1,0);records.append(h.snapshot(1810960))
 session.command(3,2,1);session.command(3,12,0);records.append(h.snapshot(1810960));session.command(3,60,0);records.append(h.snapshot(1810960))
 shot(str(w/'world-bridge/motion.png'),exe='DARKSOULS.exe',scale=.5)
finally:
 es=session.close();(w/'world-bridge/motion-events.jsonl').write_text(''.join(json.dumps(e)+'\n' for e in es));(w/'world-bridge/motion.json').write_text(json.dumps(records,indent=2));d.close();h.close()
print([r['player']['position'] for r in records])
