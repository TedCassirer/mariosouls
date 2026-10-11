from pathlib import Path
import json
p=Path('outputs/Combat harness/20261010-235246/probe-events.jsonl')
es=[json.loads(x) for x in p.read_text().splitlines()]
last=None
for e in es:
 if e['type']=='observation' and 160<=e['frame']<=194:
  sig=(e['frame'],e['stamina'])
  if sig!=last:print(e['tick'],*sig);last=sig
 elif e['type'] in ('mario_attack','mario_contact','native_call_return') and 160<=e['frame']<=194:print(e)
