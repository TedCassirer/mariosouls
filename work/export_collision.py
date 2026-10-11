"""Convert only the user's installed map collision into a private triangle cache."""
import sys
from pathlib import Path
work=Path(__file__).resolve().parent
sys.path[:0]=[str(work/'soulstruct-source/src'),str(work/'python-deps')]
import struct,json
import numpy as np
from soulstruct.containers import Binder
from soulstruct.darksouls1ptde.maps import MSB
from soulstruct.utilities.maths import Matrix3
from soulstruct.havok.fromsoft.shared.map_collision import MapCollisionModel
from ds_archive import Archives

a=Archives(); cache=work/'collision';cache.mkdir(exist_ok=True)
maps=sys.argv[1:] or ['m18_00_00_00']
for map_id in maps:
 print('Reading',map_id,flush=True)
 msb_id='m12_00_00_01' if map_id=='m12_00_00_00' else map_id
 msb=MSB.from_bytes(a.read(f'/map/MapStudio/{msb_id}.msb'))
 root=f'/map/{map_id}/h{map_id[1:]}'
 b=Binder.from_bytes(a.read(root+'.hkxbhd'),bdt_data=a.read(root+'.hkxbdt'))
 entries={Path(e.path.replace('\\','/')).name.lower():e for e in b.entries}
 tris=[];parts=[]
 for part in msb.collisions:
  name=part.model.name.lower()+f'a{map_id[1:3]}.hkx.dcx'
  if name not in entries:
   candidates=[key for key in entries if key.startswith(part.model.name.lower()+'.')]
   if len(candidates)!=1: raise ValueError((part.name,candidates,list(entries)[:5]))
   name=candidates[0]
  model=MapCollisionModel.from_bytes(entries[name].get_uncompressed_data())
  rot=np.array(Matrix3.from_euler_angles_deg(part.rotate).data)
  translation=np.array(list(part.translate))
  count=0
  for mesh in model.meshes:
   verts=mesh.vertices3D@rot.T+translation
   tri=verts[mesh.face_vertex_indices].astype('<f4')
   tris.append(tri);count+=len(tri)
  parts.append({'name':part.name,'triangles':count})
 arr=np.concatenate(tris)
 (cache/f'{map_id}.bin').write_bytes(b'MSC1'+struct.pack('<I',len(arr))+arr.tobytes())
 (cache/f'{map_id}.json').write_text(json.dumps({'triangles':len(arr),'bounds':[arr.min(axis=(0,1)).tolist(),arr.max(axis=(0,1)).tolist()],'parts':parts},indent=2))
 print(map_id,len(arr),'triangles',flush=True)
