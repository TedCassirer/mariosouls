from pathlib import Path
import sys,time,math,json
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
def objects(h):
 root=h.u(0x137d5f8);base=h.u(root+0x20);out=[]
 for i in range(h.i(root+0x1c)):
  m=base+i*0xec;n=h.i(m+0x3c);p=h.u(m+0x40)
  if not 0<n<10000:continue
  for j in range(n):
   o=p+j*0xc0;hp=int.from_bytes(h.read(o+0x2a,2),'little',signed=True)
   if hp>=0:out.append(dict(address=hex(o),index=j,hp=hp,flags=h.read(o+0x2c,4).hex(),position=[h.f(o+off) for off in [0x6c,0x7c,0x8c]]))
 return out
r=Runner(int(sys.argv[1]));record={}
try:
 r.native_off(1810800);r.warp([-2.7,198.25,-12.96],math.pi/2,1810800);time.sleep(.7);r.capture('native pot before');record['before']=objects(r.host)
 for i in range(3):
  r.observe(1810800);r.drive.key('0x48','down');time.sleep(.12);r.drive.key('0x48','up');time.sleep(.8)
 record['after']=objects(r.host);r.capture('native pot after')
finally:
 save(r.out/'native-object.json',record);save(r.out/'setup.json',r.trace)
 try:r.native_off(1810800);r.warp([3.1,196,5.7],math.pi,1810800)
 except Exception as e:print('cleanup',e)
 r.close()
print(r.out)
