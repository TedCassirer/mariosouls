"""Load only the recognized test slot using observed menu templates."""
from pathlib import Path
import sys,time,subprocess,json
import numpy as np
from PIL import Image
from um.win import Drive,shot,processes
w=Path(__file__).resolve().parent
def pixels(path):return np.asarray(Image.open(path).convert('RGB').resize((960,540)),dtype=np.float32)
refs={name:pixels(w/file) for name,file in {
 'title':'continue_small.png','main':'title-long_small.png',
 'slots':'load-slot_small.png','selected':'slot-test_small.png','notice':'quit-warning_small.png',
 'autosave':'autosave-notice_small.png'}.items()}
def err(p,key,box):
 x,y,r,b=box;return float(np.mean(abs(p[y:b,x:r]-refs[key][y:b,x:r])))
def inkerr(p,key,box):
 x,y,r,b=box;ref=refs[key][y:b,x:r];current=p[y:b,x:r];mask=np.min(ref,axis=2)>150
 return float(np.mean(abs(current[mask]-ref[mask]))) if mask.any() else 255
if '--launch' in sys.argv:subprocess.run([r'C:\Program Files (x86)\Steam\steam.exe','-applaunch','211420'])
started=time.monotonic();last_action='';d=None;events=[]
try:
 while time.monotonic()-started<75:
  live=processes('DARKSOULS')
  if not live:time.sleep(.5);continue
  if d is None:d=Drive('DARKSOULS');d.focus()
  log=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\mariosouls.log').read_text(errors='replace')
  if 'player=00000000' not in log.splitlines()[-1] and 'state active=' in log.splitlines()[-1]:
   elapsed=time.monotonic()-started;print('World loaded after %.1f seconds'%elapsed,flush=True);(w/'fast-load-timing.json').write_text(json.dumps({'seconds':elapsed,'events':events},indent=2));break
  frame=w/'fast-load-observe.png'
  try:shot(str(frame),exe='DARKSOULS.exe',scale=.5)
  except (Exception,SystemExit):time.sleep(.5);continue
  p=pixels(frame)
  action=None
  # Name + location of the test save must match before any save confirmation.
  slot_box=(260,219,794,244)
  if err(p,'autosave',(300,192,665,371))<12 and inkerr(p,'autosave',(300,192,665,371))<45:action='acknowledge-autosave-notice'
  elif err(p,'notice',(310,171,650,339))<12 and inkerr(p,'notice',(310,171,650,339))<45:action='acknowledge-quit-notice'
  elif min(err(p,'slots',slot_box),err(p,'selected',slot_box))<12:
   choice=err(p,'selected',(530,248,780,272));other=err(p,'slots',(530,248,780,272))
   if choice<12 and choice<other:action='load-test'
   elif other<12 and other<choice:action='select-test'
  elif err(p,'main',(390,361,572,479))<10 and inkerr(p,'main',(390,361,572,479))<45:action='load-menu'
  elif err(p,'title',(365,416,590,435))<10 and inkerr(p,'title',(365,416,590,435))<45:action='start'
  print('%.1fs %s'%(time.monotonic()-started,action or 'waiting'),flush=True)
  events.append({'seconds':round(time.monotonic()-started,2),'screen':action or 'waiting'})
  if '--inspect' in sys.argv:
   import shutil
   shutil.copy2(frame,w/('startup-%02d.png'%len(events)))
  if action:
   d.focus();d.hold('0x28' if action=='select-test' else '0x0D',140 if action=='select-test' else 600)
   time.sleep(.5);last_action=action
  else:time.sleep(.7)
 else:raise RuntimeError('Unrecognized screen; stopped without choosing a save.')
finally:
 if d:d.close()
