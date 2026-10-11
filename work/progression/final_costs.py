from pathlib import Path
import json
p=Path('outputs/Combat harness/20261011-001237')
for f in sorted(p.glob('*-result.json')):
 r=json.loads(f.read_text());a=r['starting_conditions'];b=r.get('ending_conditions',{});print(r['test'],r['stamina_charged'],r['stamina_decreases'],'souls',a['stats']['souls'],b.get('stats',{}).get('souls'))
