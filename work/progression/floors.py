from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'combat-harness'))
from geometry import Geometry
import math,json
g=Geometry()
for x,z,y in [(39,z,y) for z in [9,11,13,15] for y in [194.6,193.8,196,198]]+[(5.68,-31,209),(15.58,-19,208.55),(3,-12,198.2)]:
 try:print(x,y,z,round(g.floor([x,y,z]),4))
 except Exception:pass
for ident in [1810112,1810113,1810104]:
 n=next(r['position'] for r in json.loads((Path(__file__).parent/'encounters.json').read_text()) if r['id']==ident);found=[]
 for radius in [4,3,2,1]:
  for a in range(0,360,30):
   p=[n[0]+radius*math.sin(math.radians(a)),n[1]+.2,n[2]+radius*math.cos(math.radians(a))]
   try:
    y=g.floor(p);p[1]=y+.15
    if not g.blocked([p[0],p[1]+.7,p[2]],[n[0],n[1]+.7,n[2]]):found.append([radius,a,*[round(v,4) for v in p]])
   except Exception:pass
 print(ident,found[:14])
