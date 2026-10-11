import sys,configparser,time,os
from pathlib import Path
p=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\mariosouls.ini')
c=configparser.ConfigParser();c.optionxform=str;c.read(p)
if 'World' not in c:c['World']={}
for a in sys.argv[1:]:k,v=a.split('=',1);c['World'][k]=v
t=p.with_suffix('.world.tmp')
with t.open('w') as f:c.write(f)
for i in range(20):
 try:os.replace(t,p);break
 except PermissionError:time.sleep(.03)
else:raise RuntimeError('Config locked')
print(dict(c['World']))
