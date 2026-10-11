from pathlib import Path
import sys,json
w=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(w),str(w/'soulstruct-source/src'),str(w/'python-deps')]
from ds_archive import Archives
from soulstruct.darksouls1ptde.maps import MSB
from soulstruct.darksouls1ptde.params import GameParamBND
a=Archives();m=MSB.from_bytes(a.read('/map/MapStudio/m18_01_00_00.msb'));g=GameParamBND.from_bytes(a.read('/param/GameParam/GameParam.parambnd.dcx'))
for i in range(990050,990058):assert i not in g.PlayerAttacks.rows and i not in g.PlayerBehaviors.rows
rows=[]
for p in m.characters:
 if p.entity_id>0:
  row=dict(id=p.entity_id,name=p.name,model=p.model.name if p.model else '',position=list(p.translate),rotation=list(p.rotate),npc=p.character_id,ai=p.ai_id)
  rows.append(row)
(w/'progression/encounters.json').write_text(json.dumps(rows,indent=2))
print(json.dumps(rows,indent=2))
