from pathlib import Path
import configparser,time,os,sys,json
p=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\mariosouls.ini')
c=configparser.ConfigParser();c.optionxform=str;c.read(p)
if 'Combat' not in c:c['Combat']={}
limits={'FixedPower':(0,1200),'StatScaling':(0,1),'UpgradeLevel':(0,15),'EquipmentCostPerWeight':(0,.02),'stomp_cost':(0,100)}
for arg in sys.argv[1:]:
 k,v=arg.split('=',1);assert k in limits and limits[k][0]<=float(v)<=limits[k][1]
 c['Combat'][k]=v
t=p.with_suffix('.combat.tmp')
with t.open('w') as f:c.write(f)
for attempt in range(40):
 try:os.replace(t,p);break
 except PermissionError:
  if attempt==39:raise
  time.sleep(.025)
time.sleep(2.2);print(json.dumps(dict(c['Combat'])))
