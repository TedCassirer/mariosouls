from pathlib import Path
import sys,json
w=Path(__file__).parent
sys.path[:0]=[str(w/'soulstruct-source/src'),str(w/'python-deps')]
from ds_archive import Archives
from soulstruct.darksouls1ptde.params import GameParamBND
a=Archives()
for name in ['/param/GameParam/GameParam.parambnd','/param/GameParam/GameParam.parambnd.dcx']:
 try:data=a.read(name);break
 except KeyError:continue
else:raise RuntimeError('GameParam archive not located')
g=GameParamBND.from_bytes(data)
rows={k:{'name':r.Name,'data':r.to_dict()} for k,r in g.PlayerAttacks.rows.items()}
(w/'attack-params-private.json').write_text(json.dumps(rows,indent=2,default=str))
for k,r in g.PlayerAttacks.rows.items():
 if k<500 or any(t in r.Name.lower() for t in ('fist','kick','punch','素手')):print(k,r.Name)
for k,r in g.PlayerBehaviors.rows.items():
 if r.ReferenceType==0 and r.ReferenceID in (40,41,44,302): print('BEHAVIOR',k,r.ReferenceID,r.Name)
