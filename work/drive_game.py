"""Menu input with a release interval so the game's 30 Hz polling sees each tap."""
import sys,time
from um.win import Drive
d=Drive('DARKSOULS')
try:
 print('focus:',d.focus())
 for arg in sys.argv[1:]:
  if arg.startswith('wait='):time.sleep(float(arg.split('=')[1]));continue
  key,_,ms=arg.partition(':')
  print(key,d.hold(key,int(ms or '140')))
  time.sleep(.3)
finally:d.close()
