from pathlib import Path
import json
p=Path('outputs/Combat harness/20261011-000050/dive-events.jsonl')
for e in map(json.loads,p.read_text().splitlines()):
 if e['type']=='input_frame' and 50<=e['frame']<=70:print(e['frame'],e['buttons'],e['y'],hex(e['action']),e['speed'])
