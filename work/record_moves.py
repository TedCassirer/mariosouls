from pathlib import Path
import configparser,time,os
from um.win import Recorder
w=Path(__file__).resolve().parent
path=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\mariosouls.ini')
def step(frames=0,buttons=0,x=0,y=0,mode=0,wait=None):
 cfg=configparser.ConfigParser();cfg.optionxform=str;cfg.read(path);cfg['Test'].update(Serial=str(int(cfg['Test']['Serial'])+1),Mode=str(mode),Frames=str(frames),X=str(x),Y=str(y),Buttons=str(buttons))
 tmp=path.with_suffix('.tmp')
 with tmp.open('w') as f:cfg.write(f)
 os.replace(tmp,path);time.sleep(wait if wait is not None else frames/30+.28)
step(mode=1,wait=1)
r=Recorder(exe='DARKSOULS.exe',out=str(w/'mario-movement-take'),max_width=1280,audio=True).start()
try:
 time.sleep(1)
 step(4,1);step(0,wait=1.4)
 step(3,1,wait=.22);step(24,4);step(0,wait=.8)
 step(8,2);step(0,wait=.7)
 step(8,2);step(0,wait=.7)
 step(12,0,y=-1);step(12,5,y=-1);step(0,wait=1.5)
 step(5,1,x=1);step(4,0,x=1);step(5,1,x=-1);step(0,wait=1.5)
finally:step();r.stop()
