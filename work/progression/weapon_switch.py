from pathlib import Path
import sys,time,json,math
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,save
r=Runner(int(sys.argv[1]));result={}
def snap():
 s=r.observe();p=r.host.chain(0x1378700,0,8)
 s['assembly_selection']={hex(o):r.host.i(p+o) for o in range(0x224,0x24c,4)}
 return s
try:
 r.reset();r.native_off(1810101);r.warp([3.1,196,5.7],math.pi,1810101)
 result['before']=snap();r.capture('weapon-before')
 # Owner's DarkSouls.ini: KeyConfigEquip.ChangeRightWep=86 (V).
 r.drive.hold('0x56',140);time.sleep(1)
 result['after']=snap();r.capture('weapon-after')
 save(r.out/'weapon-switch.json',result)
 print({phase:result[phase]['assembly_selection'] for phase in result});print(r.out)
finally:r.close()
