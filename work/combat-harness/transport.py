"""Leased frame commands and append-only telemetry for the opt-in plugin harness."""
from pathlib import Path
import configparser,os,threading,time,secrets,json,ctypes
from host import GAME,k,user,StateError
BASE=GAME/'MarioSouls';CONFIG=BASE/'harness.ini';EVENTS=BASE/'harness-events.jsonl';STOP=BASE/'harness.stop'
def atomic_config(cfg):
 tmp=CONFIG.with_name('harness-'+str(os.getpid())+'.tmp')
 with tmp.open('w',encoding='ascii') as f:cfg.write(f)
 for attempt in range(50):
  try:os.replace(tmp,CONFIG);break
  except PermissionError:
   if attempt==49:raise
   time.sleep(.01)
def stop():
 STOP.write_text('cancel',encoding='ascii');cfg=configparser.ConfigParser();cfg['Harness']={'Armed':'0','Session':'0'};atomic_config(cfg)
class Session:
 def __init__(self,target=1810101,require_controller=None,suppress_outgoing=False,pid=None):
  self.id=secrets.randbelow(0x7ffffffe)+1;self.serial=1;self.events=[];self.offset=EVENTS.stat().st_size if EVENTS.exists() else 0;self.pending=b'';self.running=True;self.lock=threading.Lock();self.error=None;self.started=time.monotonic()
  if STOP.exists():raise StateError('Cancelled by stop command')
  foreground_pid=ctypes.c_ulong();user.GetWindowThreadProcessId(user.GetForegroundWindow(),ctypes.byref(foreground_pid));self.pid=pid or foreground_pid.value
  self.cfg=configparser.ConfigParser();self.cfg.optionxform=str
  self.cfg['Harness']={k:str(v) for k,v in dict(Armed=1,Session=self.id,Serial=1,Target=target,Mode=0,Frames=0,Buttons=0,X=0,Y=0,Heartbeat=k.GetTickCount()&0xffffffff).items()}
  if require_controller is not None:self.cfg['Harness']['RequireController']=str(int(require_controller))
  self.cfg['Harness']['SuppressOutgoing']=str(int(suppress_outgoing))
  self.write();self.thread=threading.Thread(target=self.heartbeat,daemon=True);self.thread.start()
  try:self.wait(lambda e:e['type']=='armed',3)
  except BaseException:
   self.close();raise
 def write(self):
  self.cfg['Harness']['Heartbeat']=str(k.GetTickCount()&0xffffffff);atomic_config(self.cfg)
 def heartbeat(self):
  try:
   while self.running:
    with self.lock:
     if STOP.exists() or time.monotonic()-self.started>115:self.cfg['Harness']['Armed']='0';self.running=False
     foreground_pid=ctypes.c_ulong();user.GetWindowThreadProcessId(user.GetForegroundWindow(),ctypes.byref(foreground_pid))
     if foreground_pid.value!=self.pid:
      self.cfg['Harness'].update(Armed='0',StopReason='focus_lost');self.running=False
     self.write()
    time.sleep(.25)
  except Exception as e:self.error=str(e);self.running=False
 def poll(self):
  if EVENTS.exists():
   with EVENTS.open('rb') as f:f.seek(self.offset);data=f.read();self.offset=f.tell()
   self.pending+=data;lines=self.pending.split(b'\n');self.pending=lines.pop()
   for line in lines:
    try:e=json.loads(line)
    except (ValueError,UnicodeError):continue
    if e.get('session')==self.id:self.events.append(e)
  return self.events
 def wait(self,pred,timeout=7):
  until=time.monotonic()+timeout;start=len(self.events)
  while time.monotonic()<until:
   self.poll()
   # Search whole session so fast responses received during command writes are not lost.
   found=next((e for e in reversed(self.events) if pred(e)),None)
   if found:return found
   stopped=next((e for e in reversed(self.events) if e['type']=='stopped'),None)
   if stopped:raise StateError('Harness stopped: '+stopped['reason'])
   if self.error:raise StateError('Heartbeat failed: '+self.error)
   time.sleep(.02)
  raise StateError('Timed out waiting for harness acknowledgement')
 def command(self,mode,frames=0,buttons=0,x=0,y=0,delay=0):
  if STOP.exists():raise StateError('Cancelled by stop command')
  self.serial+=1
  with self.lock:
   self.cfg['Harness'].update({k:str(v) for k,v in dict(Serial=self.serial,Mode=mode,Frames=frames,Buttons=buttons,X=x,Y=y,ButtonDelay=delay).items()});self.write()
  serial=self.serial;self.wait(lambda e:e['type']=='command_accepted' and e['command']==serial,3)
  if mode==3:self.wait(lambda e:e['type']=='command_frames_complete' and e['command']==serial,frames/30+3)
  return self.poll()
 def close(self):
  if not hasattr(self,'thread'):return
  self.running=False
  with self.lock:self.cfg['Harness']['Armed']='0';self.write()
  self.thread.join(1)
  end=time.monotonic()+1
  while time.monotonic()<end:
   self.poll()
   if any(e['type']=='stopped' for e in self.events):break
   time.sleep(.03)
  return self.events
