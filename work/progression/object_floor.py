from pathlib import Path
import sys
w=Path(__file__).resolve().parents[1];sys.path[:0]=[str(w),str(w/'soulstruct-source/src'),str(w/'python-deps')]
from ds_archive import Archives
from soulstruct.containers import Binder
a=Archives()
for path in ['/obj/o8510.objbnd','/obj/o8510.objbnd.dcx']:
 try:b=Binder.from_bytes(a.read(path));break
 except KeyError:pass
for e in b.entries:
 print(e.path,len(e.get_uncompressed_data()))
 if e.path.lower().endswith('.hkx'):(w/'progression'/Path(e.path.replace('\\','/')).name).write_bytes(e.get_uncompressed_data())
import struct,numpy as np
from soulstruct.havok.fromsoft.shared.map_collision import MapCollisionModel
m=MapCollisionModel.from_path(w/'progression/o8510.hkx')
tri=np.concatenate([(mesh.vertices3D+np.array([3.197979927,197.505523682,-21.538074493]))[mesh.face_vertex_indices] for mesh in m.meshes]).astype('<f4')
(w/'progression/asylum-arena-floor.bin').write_bytes(b'MSC1'+struct.pack('<I',len(tri))+tri.tobytes())
print('Floor bounds',tri.min(axis=(0,1)),tri.max(axis=(0,1)),len(tri))

