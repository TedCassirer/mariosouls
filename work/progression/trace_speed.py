from pathlib import Path
import json
p=Path('outputs/Combat harness/20261010-235453/dive-events.jsonl')
for e in map(json.loads,p.read_text().splitlines()):
 if e['type']=='input_frame' and 40<=e['frame']<=70:print(e['frame'],e['command'],e['buttons'],e['y'],hex(e['action']),e['speed'])
p=Path('outputs/Combat harness/20261010-235440/00-punch_in-events.jsonl')
last=None
for e in map(json.loads,p.read_text().splitlines()):
 if e['type']=='observation' and e['stamina']!=last: print(e['frame'],e['stamina']);last=e['stamina']
