"""Read-only PTDE BHD5 archive reader. Format reference: JKAnderson/SoulsFormats BHD5.cs."""
from pathlib import Path
import struct
GAME=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
class Archives:
 def __init__(self):
  self.entries={}
  for i in range(4):
   bhd=(GAME/f'dvdbnd{i}.bhd5').read_bytes()
   assert bhd[:5]==b'BHD5\xff'
   count,offset=struct.unpack_from('<ii',bhd,16)
   for b in range(count):
    n,ptr=struct.unpack_from('<ii',bhd,offset+b*8)
    for j in range(n):
     hashed,size,pos=struct.unpack_from('<IIQ',bhd,ptr+j*16)
     assert pos+size<=(GAME/f'dvdbnd{i}.bdt').stat().st_size
     self.entries[hashed]=(i,size,pos)
 @staticmethod
 def hash(path):
  h=0
  for ch in ('/'+path.replace('\\','/').lower().lstrip('/')):h=(h*37+ord(ch))&0xffffffff
  return h
 def read(self,path):
  i,size,pos=self.entries[self.hash(path)]
  with (GAME/f'dvdbnd{i}.bdt').open('rb') as f:f.seek(pos);return f.read(size)
if __name__=='__main__':
 a=Archives(); print('Archive entries:',len(a.entries))
 for map_id in ['m10_02_00_00','m18_00_00_00','m10_01_00_00']:
  path=f'/map/MapStudio/{map_id}.msb'
  data=a.read(path); print(path,len(data),data[:16]);(Path(__file__).parent/(map_id+'.msb')).write_bytes(data)
