from pathlib import Path
import sys
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from runner import Runner,RECIPES
RECIPES['stomp']=[(12,1,0,-.7),(8,0,0,.7),(45,0,0,0)]
r=Runner(int(sys.argv[1]))
try:r.run('stomp',0)
finally:r.close()
print(r.out)
