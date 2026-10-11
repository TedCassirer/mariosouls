from pathlib import Path
import sys,struct,json
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from host import Host
h=Host(int(sys.argv[1]));h.snapshot();repo=h.u(0x13785b4)
for i in range(36):
 try:
  p=h.chain(repo+36*i+12,0,0x20)
  header=h.read(p,48)
  print(i,hex(p),struct.unpack_from('<H',header,10)[0],header[12:44].split(b'\0')[0])
 except Exception:pass
h.close()
