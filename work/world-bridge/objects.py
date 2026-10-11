from pathlib import Path
import sys,struct,json
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from host import Host,StateError
h=Host(int(sys.argv[1]));h.snapshot();root=h.u(0x137d5f8);n=h.i(root+0x1c);base=h.u(root+0x20);print('root',hex(root),'maps',n,hex(base));out=[]
assert 0<n<100
for i in range(n):
 m=base+i*0xec;count=h.i(m+0x3c);p=h.u(m+0x40)
 if not 0<count<10000:continue
 print('map',i,'objects',count,hex(p))
 names={};nc=h.i(m+0x54);nt=h.u(m+0x58)
 for k in range(max(0,min(nc,10000))):
  key=h.i(nt+k*8);index=h.u(nt+k*8+4)&0xfffff;names[index]=key
 for j in range(count):
  o=p+j*0xc0;b=h.read(o,0xc0);words=struct.unpack('<48I',b);vals=struct.unpack('<48f',b)
  ids=[(j,names[j])] if j in names else []
  if ids:
   out.append(dict(address=hex(o),ids=ids,words=[hex(v) for v in words],floats=[v if abs(v)<10000 else None for v in vals]));print(hex(o),ids,'param',hex(words[9]),'flags',b[0x2c:0x30].hex(),'hp',struct.unpack_from('<h',b,0x2a)[0])
(w/'world-bridge/objects.json').write_text(json.dumps(out,indent=2));h.close()
