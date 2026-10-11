from pathlib import Path
import sys,json,dataclasses
w=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(w),str(w/'soulstruct-source/src'),str(w/'python-deps')]
from ds_archive import Archives
from soulstruct.darksouls1ptde.maps import MSB
m=MSB.from_bytes(Archives().read('/map/MapStudio/m18_01_00_00.msb'))
rows=[]
for p in m.characters:
 if 1810100<=p.entity_id<=1810115:
  row={f.name:str(getattr(p,f.name)) for f in dataclasses.fields(p) if not f.name.startswith('_')}
  rows.append(row)
(Path(__file__).parent/'encounter-data.json').write_text(json.dumps(rows,indent=2))
for row in rows:print(json.dumps(row))
from soulstruct.darksouls1ptde.params import GameParamBND
g=GameParamBND.from_bytes(Archives().read('/param/GameParam/GameParam.parambnd.dcx'))
for i in [250020,250021,250022]:
 r=g.Characters[i]
 print(i,r.DisableRespawnOnRest)


