from pathlib import Path
import json
p=Path('outputs/Combat harness/20261010-235849')
for f in sorted(p.glob('*-result.json')):
 r=json.loads(f.read_text());es=[json.loads(x) for x in (p/r['events_file']).read_text().splitlines()];s=[e['stamina'] for e in es if e['type']=='observation'];print(r['test'],r['damage_to_target'],'charged',r['stamina_charged'],'observed min',min(s) if s else None,'sampled',r['stamina_decreases'])
