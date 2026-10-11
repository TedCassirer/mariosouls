from pathlib import Path
import sys
w=Path(__file__).parent;sys.path[:0]=[str(w/'soulstruct-source/src'),str(w/'python-deps')]
from ds_archive import Archives
from soulstruct.containers import Binder
from soulstruct.darksouls1ptde.maps import MSB
a=Archives()
for folder in ['m12_00_00_00','m12_00_00_01']:
 for stem in ['h12_00_00_00','h12_00_00_01']:
  root=f'/map/{folder}/{stem}'
  try:
   b=Binder.from_bytes(a.read(root+'.hkxbhd'),bdt_data=a.read(root+'.hkxbdt'));print('COMBINATION',root,len(b.entries),[e.path for e in b.entries if '0125' in e.path])
  except KeyError:pass
for mid in ['m12_00_00_00','m12_00_00_01']:
 for kind in ['h','l']:
  root=f'/map/{mid}/{kind}{mid[1:]}'
  try:
   b=Binder.from_bytes(a.read(root+'.hkxbhd'),bdt_data=a.read(root+'.hkxbdt'))
   print(root,len(b.entries),[e.path for e in b.entries if '0125' in e.path])
  except KeyError:print('Absent',root)
for root in ['/map/m12_00_00_00/h0125B0A12.hkx.dcx','/map/m12_00_00_01/h0125B0A12.hkx.dcx','/map/m12_00_00_00/h0125B0A12.hkx']:
 try:print('Standalone',root,len(a.read(root)))
 except KeyError:pass
msb=MSB.from_bytes(a.read('/map/MapStudio/m12_00_00_01.msb'))
for part in msb.collisions:
 if part.model.name.lower()=='h0125b0':print(part.to_dict())
