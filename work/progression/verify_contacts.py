from pathlib import Path
import json
p=Path('outputs/Combat harness/20261010-235849')
for f in sorted(p.glob('*-result.json')):
 r=json.loads(f.read_text());events=[json.loads(x) for x in (p/r['events_file']).read_text().splitlines()];stam=[e['stamina'] for e in events if e['type']=='observation'];pairs=[(e['serial'],e['target_id']) for e in events if e['type']=='mario_contact'];print(r['test'],r['damage_to_target'],r['stamina_charged'],min(stam),len(pairs)==len(set(pairs)))
