from pathlib import Path
import json,collections
for folder in ['20261010-235246','20261011-000845']:
 es=json.loads((Path('outputs/Combat harness')/folder/'probe-trace.json').read_text());cs=collections.Counter()
 for e in es:
  t=e.get('target')
  if t:
   for slot,a in t['tracks']:
    if slot==14:cs[a]+=1
 print(folder,cs)
