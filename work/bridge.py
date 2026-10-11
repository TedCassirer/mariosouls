"""Private test control for the MarioSouls prototype. Does not send desktop input."""
from pathlib import Path
import configparser,os,sys
path=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\mariosouls.ini')
cfg=configparser.ConfigParser();cfg.optionxform=str;cfg.read(path,encoding='ascii')
if 'Test' not in cfg:cfg['Test']={}
cfg['Test'].update({'Serial':str(int(cfg['Test'].get('Serial','0'))+1),'Mode':'0','Frames':'0','Buttons':'0','X':'0','Y':'0'})
for arg in sys.argv[1:]:
 key,value=arg.split('=',1)
 cfg['MarioSouls' if key in ('RenderMode','DepthTest','HideNative','DisableMapHit','EnableCombat') else 'Test'][key]=value
tmp=path.with_suffix('.tmp')
with tmp.open('w',encoding='ascii') as f:cfg.write(f)
os.replace(tmp,path)
print(dict(cfg['Test']))
