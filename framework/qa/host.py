"""PTDE observation, with fresh identity checks at every operation.
Only the explicitly requested player warp uses write access. No HP/save/AI writes.
"""
import ctypes as c,struct,math,time,json,sys,re
from ctypes import wintypes as wt
from pathlib import Path
WORK=Path(__file__).resolve().parents[2]/'work'
sys.path.insert(0,str(WORK/'python-deps'))
import pefile
GAME=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
MENU_FIELDS=[0x24,0x28,0x3c,0x40,0x48,0x4c,0x50,0x5c,0x60,0x64,0x68,0x70,0x78,0x80,0x84,0x88,0x98,0x9c,0xa0,0xac,0xbc,0xc4,0xd8,0xe8,0xec,0xfc,0x108,0x118,0x11c,0x120,0x210,0x2c4]
k=c.WinDLL('kernel32',use_last_error=True);user=c.WinDLL('user32',use_last_error=True)
k.OpenProcess.argtypes=[wt.DWORD,wt.BOOL,wt.DWORD];k.OpenProcess.restype=wt.HANDLE
k.ReadProcessMemory.argtypes=[wt.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
k.WriteProcessMemory.argtypes=k.ReadProcessMemory.argtypes;k.CloseHandle.argtypes=[wt.HANDLE]
user.GetForegroundWindow.restype=wt.HWND;user.GetWindowThreadProcessId.argtypes=[wt.HWND,c.POINTER(wt.DWORD)]
class StateError(RuntimeError):pass
class Host:
 def __init__(self,pid,write=False):
  self.pid=pid;self.write_enabled=write;self.h=k.OpenProcess(0x1038 if write else 0x1010,False,pid)
  if not self.h:raise StateError('Cannot open game process')
  if self.u(0x400080)!=0xfc293654:raise StateError('Unexpected executable version')
  self.protection_slots=self.find_protections()
 def close(self):
  if self.h:k.CloseHandle(self.h);self.h=None
 def read(self,a,n):
  if a<0x10000:raise StateError('Loading/null pointer')
  b=c.create_string_buffer(n);got=c.c_size_t()
  if not k.ReadProcessMemory(self.h,a,b,n,c.byref(got)) or got.value!=n:raise StateError('Stale/unreadable game reference')
  return b.raw
 def u(self,a):return struct.unpack('<I',self.read(a,4))[0]
 def i(self,a):return struct.unpack('<i',self.read(a,4))[0]
 def f(self,a):return struct.unpack('<f',self.read(a,4))[0]
 def v(self,a):return list(struct.unpack('<fff',self.read(a,12)))
 def chain(self,a,*offsets):
  for o in offsets:a=self.u(a+o)
  return a
 def focused(self):
  p=wt.DWORD();user.GetWindowThreadProcessId(user.GetForegroundWindow(),c.byref(p));return p.value==self.pid
 def find_protections(self):
  pe=pefile.PE(str(GAME/'DARKSOULS.exe'));sec=pe.sections[0];data=self.read(0x400000+sec.VirtualAddress,sec.Misc_VirtualSize)
  out={}
  # Start after the setter prologue, which the host stamina hook now detours.
  for name,pat,off in [('player_no_dead','53 56 8B F0 8A 9E C4 03 00 00 8B 06 8B 90 A4 00 00 00 C0 EB 05 8B CE 80 E3 01 FF D2 84 C0 ? ? 80 3D ? ? ? ? 00',0x22),('global','3B 8A E4 02 00 00 ? ? F6 82 C5 03 00 00 04 ? ? 80 3D ? ? ? ? 00',0x13)]:
   rx=b''.join(b'.' if x=='?' else re.escape(bytes([int(x,16)])) for x in pat.split());hits=list(re.finditer(rx,data,re.S))
   if len(hits)!=1:raise StateError('Protection signature is not unique: '+name)
   out[name]=struct.unpack_from('<I',data,hits[0].start()+off)[0]
  camera=list(re.finditer(b'\\xd9\\x45\\x08\\xa1....\\x51\\xd9\\x1c\\x24\\x50',data,re.S))
  if len(camera)!=1:raise StateError('Camera signature is not unique')
  out['camera']=struct.unpack_from('<I',data,camera[0].start()+4)[0]
  return out
 def camera_forward(self):
  return self.v(self.chain(self.protection_slots['camera'],0,0x3c,0x60)+0x30)
 def tracks(self,p):
  t=self.chain(p,0x28,0x18)
  return [[j,self.i(t+j*0x9c)] for j in range(29) if self.u(t+j*0x9c+4)!=0xffffffff]
 def actor(self,p):
  pos=self.chain(p,0x28,0x1c)
  return dict(reference=hex(p),id=self.i(p+0x208),model=self.read(p+0x38,20).decode('utf-16-le',errors='replace').split('\0')[0],team=self.i(p+0x74),hp=self.i(p+0x2d4),hp_max=self.i(p+0x2d8),stamina=self.i(p+0x2e4),poise=self.f(p+0x1c0),poise_max=self.f(p+0x1c4),position=self.v(pos+0x10),facing=self.f(pos+4),flags1=self.u(p+0x1fc),flags2=self.u(p+0x3c4),tracks=self.tracks(p))
 def snapshot(self,target_id=1810101,require_test=True):
  stats=self.chain(0x1378700,0,8);name=self.read(stats+0xa0,40).decode('utf-16-le',errors='replace').split('\0')[0]
  level=self.i(stats+0x88)
  melee_fixture=level==13 and [self.i(stats+o) for o in (0x38,0x40,0x48,0x50,0x58,0x80,0x60,0x68)]==[11,11,11,16,13,11,11,11]
  if require_test and (name!='Mario QA' or not (1<=level<=10 or melee_fixture)):raise StateError('Requires Mario QA low-level or exact melee fixture: '+name)
  world=self.u(0x137e204);raw=self.read(world+0xa12,2);mapid=raw[1]*100+raw[0]
  if raw==b'\xff\xff':raise StateError('Loading/map transition')
  if require_test and mapid!=1801:raise StateError('Unexpected map: '+str(mapid))
  root=self.u(0x137dc70);begin=self.u(root+4);end=self.u(root+8)
  if not begin or not 4<=end-begin<=16384:raise StateError('Loading/invalid character table')
  p=self.u(begin);a=self.actor(p);a.update(name=name,script_id=10000)
  candidates=[];nearby=[]
  for offset in range(begin+4,end,4):
   try:
    n=self.u(offset)
    if not n:continue
    if self.i(n+0x208)==target_id:candidates.append(n)
    if self.i(n+0x74) in [6,7,9] and self.i(n+0x2d4)>0:
     b=self.actor(n)
     if math.dist(a['position'],b['position'])<12:nearby.append(b)
   except StateError:continue # An unrelated NPC can stream out during a bonfire reset.
  if len(candidates)>1:raise StateError('Duplicate target game identity')
  try:target=self.actor(candidates[0]) if candidates else None
  except StateError:target=None # Reset waits for the game to reconstruct the target.
  menu=self.u(0x13786d0);menus={hex(x):self.u(menu+x) for x in MENU_FIELDS if self.u(menu+x)}
  tracks=[i%100000 for _,i in a['tracks'] if i>=0]
  dbg=self.protection_slots['global'];dbg_names=['no_stamina','no_mp','no_arrow','hide','silence','no_dead','no_damage','no_hit','no_attack','no_move','no_ai']
  protections={key:self.read(dbg+j,1)[0] for j,key in enumerate(dbg_names)}
  protections['player_no_dead']=self.read(self.protection_slots['player_no_dead'],1)[0]
  protections['player_flags1']=a['flags1']&0x0e000000;protections['player_flags2']=a['flags2']&0x4e0
  attributes={key:self.i(stats+off) for key,off in [('hp_max',0x14),('hp_mod_max',0x10),('stamina_max',0x30),('stamina_mod_max',0x2c),('vitality',0x38),('attunement',0x40),('endurance',0x48),('strength',0x50),('dexterity',0x58),('intelligence',0x60),('faith',0x68),('humanity',0x7c),('resistance',0x80),('level',0x88),('souls',0x8c)]}
  equipment={key:self.i(stats+off) for key,off in [('stance',0x230),('left1',0x24c),('right1',0x250),('left2',0x254),('right2',0x258),('helmet',0x26c),('chest',0x270),('gloves',0x274),('legs',0x278),('ring1',0x280),('ring2',0x284),('item1',0x288),('item2',0x28c),('item3',0x290),('item4',0x294),('item5',0x298)]}
  held_index=self.i(stats+0x238);held_right=equipment['right1'] if held_index==0 else equipment['right2'] if held_index==1 else None
  return dict(tick=k.GetTickCount()&0xffffffff,focus=self.focused(),map=mapid,player=a,target=target,target_id=target_id,nearby=nearby,menu=menus,interaction=any(7000<=v<9000 for v in tracks),native_idle=any(j==14 and i>=0 and i%100000 in (0,20000) for j,i in a['tracks']),action_prompt=bool(self.u(menu+0x58)),stats=attributes,equipment=equipment,held_right_index=held_index,held_right_id=held_right,protections=protections)
 def put(self,a,b):
  if not self.write_enabled:raise StateError('Read-only host')
  n=c.c_size_t()
  if not k.WriteProcessMemory(self.h,a,b,len(b),c.byref(n)) or n.value!=len(b):raise StateError('Native warp write failed')
 def warp(self,xyz,heading,target_id=1810101):
  if not all(math.isfinite(v) for v in [*xyz,heading]) or not(-60<xyz[0]<100 and 180<xyz[1]<230 and -100<xyz[2]<100):raise StateError('Outside Asylum test bounds')
  s=self.snapshot(target_id)
  if not s['focus'] or s['menu'] or s['interaction'] or s['player']['hp']<=0 or s['player']['flags2']&0x300:raise StateError('Warp requires focused, living native player outside interactions')
  ref=s['player']['reference']
  for _ in range(10):
   fresh=self.snapshot(target_id)
   if fresh['player']['reference']!=ref or not fresh['focus'] or fresh['player']['hp']<=0 or fresh['menu'] or fresh['interaction'] or fresh['player']['flags2']&0x300:raise StateError('Player state changed during warp')
   p=int(ref,16);m=self.u(p+0x28)
   self.put(m+0xd0,struct.pack('<fff',*xyz));self.put(m+0xe4,struct.pack('<f',heading));self.put(m+0xc8,b'\1');time.sleep(1/30)
  return self.snapshot(target_id)
if __name__=='__main__':
 h=Host(int(sys.argv[1]));print(json.dumps(h.snapshot(int(sys.argv[2]) if len(sys.argv)>2 else 1810101),indent=2));h.close()
