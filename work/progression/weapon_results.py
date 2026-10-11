from pathlib import Path
import json
for folder in ['20261011-000600','20261011-000721']:
 for f in sorted((Path('outputs/Combat harness')/folder).glob('*-result.json')):
  r=json.loads(f.read_text());s=r['starting_conditions'];print(folder,r['test'],'weapon',s.get('held_right_id'),'damage',r['damage_to_target'],'cost',r['stamina_charged'])
