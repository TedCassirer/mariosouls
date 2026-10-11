from pathlib import Path
import sys,json
w=Path(__file__).resolve().parents[1];sys.path[:0]=[str(w),str(w/'soulstruct-source/src'),str(w/'python-deps')]
from ds_archive import Archives
from soulstruct.darksouls1ptde.params import GameParamBND
g=GameParamBND.from_bytes(Archives().read('/param/GameParam/GameParam.parambnd.dcx'))
print('Player row counts',len(g.PlayerAttacks.rows),len(g.PlayerBehaviors.rows))
for n in [250020,250021,250022,255030,223200]:
 d=g.Characters[n].to_dict();print(n,{k:v for k,v in d.items() if any(t in k.lower() for t in ['health','defen','poise','respawn'])})
print('Armor',[(i,{k:v for k,v in g.Armor[i].to_dict().items() if 'weight' in k.lower()}) for i in [210000,211000,212000,213000]])
