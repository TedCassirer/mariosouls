from pathlib import Path
import json
p=Path('outputs/Combat harness/20261011-000754/probe-events.jsonl');es=list(map(json.loads,p.read_text().splitlines()))
for i,e in enumerate(es):
 if e['type']=='confirmed_player_hp_loss':
  print(e)
  print('nearby frames',[(x['frame'],hex(x['action']),x.get('vertical_speed')) for x in es[max(0,i-15):i+8] if x['type']=='input_frame'])
  print('nearby obs',[(x['frame'],x['player_hp'],hex(x['action']),x['player_pos']) for x in es[max(0,i-15):i+8] if x['type']=='observation'])
