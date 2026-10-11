"""Focused regression of the existing visible-Mario bonfire handoff; leaves Mario at the bonfire."""
import sys,time,math,subprocess
from runner import Runner,save,WORK,StateError
r=Runner(int(sys.argv[1]));record={}
try:
 r.reset();r.native_off(1810101);r.warp([3.1,196,5.7],math.pi,1810101)
 subprocess.run([sys.executable,str(WORK/'bridge.py'),'Mode=1'],check=True,capture_output=True)
 r.until(lambda s:s['player']['flags2']&0x300==0x300,4,1810101,'normal-mario-active');time.sleep(1)
 record['before']=r.observe(label='normal-mario-before-bonfire')
 for attempt in range(2):
  r.drive.hold('0x51',140)
  try:r.until(lambda s:s['menu'].get('0x40')==2,4,1810101,'visible-mario-bonfire');break
  except StateError as e:
   if attempt or 'Timed out' not in str(e):raise
 record['bonfire']=r.observe(label='bonfire-open');r.capture('Mario stays visible at the bonfire')
 r.drive.hold('0x0D',140)
 r.until(lambda s:not s['menu'] and not s['interaction'] and s['player']['flags2']&0x300==0x300,10,1810101,'normal-mario-resumed');time.sleep(.8)
 ground=r.observe()['player']['position'][1];r.drive.hold('0x20',140)
 record['jump']=r.until(lambda s:s['player']['position'][1]>ground+.35,3,1810101,'jump-after-interaction')
 r.until(lambda s:abs(s['player']['position'][1]-ground)<.25,5,1810101,'landed');record['final']=r.observe(label='controls-returned-to-user')
 record['status']='completed';r.capture('Mario ready after tests')
finally:
 save(r.out/'interaction-regression.json',record);save(r.out/'setup.json',r.trace);r.close()
print(r.out)
