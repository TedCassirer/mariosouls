from pathlib import Path
import json
base=Path('outputs/Combat harness');runs=['20261010-235849','20261011-000600','20261011-000845','20261011-001237'];rows=[];seen=set()
for run in runs:
 for p in sorted((base/run).glob('*events.jsonl')):
  es=[json.loads(x) for x in p.read_text().splitlines()];attacks={e['serial']:e for e in es if e['type']=='mario_attack'}
  for e in es:
   if e['type']!='mario_contact':continue
   key=(run,p.name,e['serial'],e['target_id']);assert key not in seen,('duplicate',key);seen.add(key)
   a=attacks[e['serial']]
   rows.append(dict(run=run,events=p.name,move=a['move'],serial=e['serial'],target=e['target_id'],power=a['power'],stamina_cost=a['cost'],contact_after_ms=(e['tick']-a['tick'])&0xffffffff,contact_after_frames=e['frame']-a['frame']))
Path('outputs/MarioSouls attack timing measurements.json').write_text(json.dumps(rows,indent=2))
for move in dict.fromkeys(r['move'] for r in rows):
 r=next(r for r in rows if r['move']==move);print(move,r['contact_after_ms'],r['contact_after_frames'])
print('unique attack/enemy pairs',len(rows))
