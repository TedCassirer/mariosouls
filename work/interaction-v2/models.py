from pathlib import Path
import sys,time,json
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from host import Host
from um.win import Drive
h=Host(int(sys.argv[1]));d=Drive('DARKSOULS');assert d.focus();h.snapshot(1810960);rows=[]
try:
 for i in range(45):
  s=h.snapshot(1810960);assert s['focus'];p=int(s['player']['reference'],16);a=h.u(p+0x658);table=h.u(a+0x74);n=h.u(a+0x70)
  rows.append({'t':time.monotonic(),'tracks':s['player']['tracks'],'slots':[[hex(h.u(table+j*12+k)) for k in (0,4,8)] for j in range(n)],'model':h.read(a,0x140).hex()})
  if i==2:d.hold('0x45',140)
  time.sleep(.1)
finally:d.close();h.close()
Path('work/interaction-v2/model-trace.json').write_text(json.dumps(rows,indent=2))
for i,s in enumerate(rows):
 if i==0 or s['slots']!=rows[i-1]['slots']:print(i,s['tracks'],s['slots'])
