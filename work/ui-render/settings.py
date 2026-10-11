from pathlib import Path
import configparser,sys,os,time
p=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\mariosouls.ini')
c=configparser.ConfigParser();c.optionxform=str;c.read(p)
for arg in sys.argv[1:]:
 k,v=arg.split('=',1);c['MarioSouls'][k]=v
t=p.with_name('ui-settings.tmp')
with t.open('w') as f:c.write(f)
for i in range(50):
 try:os.replace(t,p);break
 except PermissionError:time.sleep(.01)
