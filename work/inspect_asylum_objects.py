from pathlib import Path
import sys,json
w=Path(__file__).resolve().parent
sys.path[:0]=[str(w/'soulstruct-source/src'),str(w/'python-deps')]
from ds_archive import Archives
from soulstruct.darksouls1ptde.maps import MSB
a=Archives();m=MSB.from_bytes(a.read('/map/MapStudio/m18_01_00_00.msb'))
objects=[dict(name=o.name,id=o.entity_id,pos=list(o.translate),rot=list(o.rotate)) for o in m.objects]
(w/'asylum-objects.json').write_text(json.dumps(objects,indent=2))
for o in objects:
 if o['id']>0 and not o['name'].startswith('o0500'):print(o)
from soulstruct.darksouls1ptde.events import EMEVD
e=EMEVD.from_bytes(a.read('/event/m18_01_00_00.emevd.dcx'))
e.write_evs(w/'asylum.evs.py')
