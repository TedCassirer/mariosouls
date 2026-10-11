import sys,json,time
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'combat-harness'))
from host import Host
def contacts(h):
 p=h.chain(0x137dc70,0,4,0);ph=h.chain(p,0x28,0x1c);proxy=h.u(ph+0x38);count=h.i(proxy+0x14);a=h.u(proxy+0x10)
 return dict(position=h.v(ph+0x10),floor=h.v(ph+0x170),contacts=[dict(point=h.v(a+i*0x30),normal=h.v(a+i*0x30+0x10),distance=h.f(a+i*0x30+0x1c),bodyA=hex(h.u(a+i*0x30+0x20)),bodyB=hex(h.u(a+i*0x30+0x28))) for i in range(max(0,min(24,count)))])
if __name__=='__main__':
 h=Host(int(sys.argv[1]));out=[]
 for i in range(int(sys.argv[2]) if len(sys.argv)>2 else 1):out.append(contacts(h));time.sleep(.1)
 print(json.dumps(out));h.close()
