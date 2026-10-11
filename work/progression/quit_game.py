"""Save via the previously observed native quit dialog, verify it, then close."""
from pathlib import Path
import sys,time,subprocess,numpy as np
from PIL import Image
w=Path(__file__).resolve().parents[1];sys.path.insert(0,str(w/'combat-harness'))
from host import Host
from um.win import Drive,shot
pid=int(sys.argv[1]);h=Host(pid);s=h.snapshot();assert not s['menu'] and not s['interaction']
subprocess.run([sys.executable,str(w/'bridge.py'),'Mode=2'],check=True);time.sleep(.5)
d=Drive('DARKSOULS');assert d.focus();time.sleep(.6)
try:
 for key in ['0x23','0x27','0x27','0x27','0x0D','0x28','0x28','0x28','0x28','0x0D']:
  assert h.focused();d.hold(key,140);time.sleep(.3)
 shot(str(w/'progression/quit-check.png'),exe='DARKSOULS.exe',scale=.5)
 ref=np.array(Image.open(w/'progression/quit2_small.png').convert('RGB')).astype(float);cur=np.array(Image.open(w/'progression/quit-check_small.png').convert('RGB')).astype(float)
 assert cur.shape==ref.shape and np.abs(ref[230:345,330:670]-cur[230:345,330:670]).mean()<12,'Quit dialog did not match; no confirmation sent'
 d.hold('0x25',140);time.sleep(.3);d.hold('0x0D',350);time.sleep(3)
 root=h.u(0x137dc70);begin=h.u(root+4) if root else 0
 assert not begin or h.u(begin)==0,'World still loaded; leaving process open'
 subprocess.run(['powershell','-NoProfile','-Command',f'$taskGame=Get-Process -Id {pid};$taskGame.CloseMainWindow() | Out-Null;$taskGame.WaitForExit(5000)'],check=True)
finally:d.close();h.close()
