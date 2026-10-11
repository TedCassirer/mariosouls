"""Small, bounded Mario input test against one identified Asylum hollow."""
from pathlib import Path
import ctypes as c,struct,sys,time,configparser,os,math
from ctypes import wintypes as w
k=c.WinDLL('kernel32');k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)];k.CloseHandle.argtypes=[w.HANDLE]
h=k.OpenProcess(0x1010,False,int(sys.argv[1]))
path=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\mariosouls.ini')
def read(a,n):
 b=c.create_string_buffer(n);got=c.c_size_t();assert k.ReadProcessMemory(h,a,b,n,c.byref(got));return b.raw
def u(a):return struct.unpack('<I',read(a,4))[0]
def v(a):return struct.unpack('<fff',read(a,12))
def step(x,y,buttons,frames):
 cfg=configparser.ConfigParser();cfg.optionxform=str;cfg.read(path);cfg['Test'].update(Serial=str(int(cfg['Test']['Serial'])+1),Mode='0',Frames=str(frames),X=str(x),Y=str(y),Buttons=str(buttons))
 tmp=path.with_suffix('.tmp')
 with tmp.open('w') as f:cfg.write(f)
 os.replace(tmp,path);time.sleep(frames/30+.24)
try:
 stats=u(u(0x1378700)+8);assert read(stats+0xa0,40).decode('utf-16-le',errors='replace').split('\0')[0]=='test'
 root=u(0x137dc70);begin=u(root+4);end=u(root+8);player=u(begin)
 target=next(u(a) for a in range(begin,end,4) if u(u(a)+0x208)==1810101)
 for i in range(20):
  hp=u(target+0x2d4)
  if hp==0 or hp>100000:print('Target defeated',flush=True);break
  pos=v(u(u(player+0x28)+0x1c)+0x10);enemy=v(u(u(target+0x28)+0x1c)+0x10)
  cam=u(u(u(0x137d6dc)+0x3c)+0x60);f=v(cam+0x30);dx,dz=enemy[0]-pos[0],enemy[2]-pos[2];distance=math.hypot(dx,dz)
  sx=-f[2]*dx+f[0]*dz;sy=-f[0]*dx-f[2]*dz
  dominant=max(abs(sx),abs(sy));x=0 if abs(sx)<dominant*.45 else (1 if sx>0 else -1);y=0 if abs(sy)<dominant*.45 else (1 if sy>0 else -1)
  print(i,'HP',hp,'range',round(distance,2),'input',x,y,flush=True)
  if distance>1:step(x,y,0,min(8,max(2,int((distance-.8)*4))))
  else:step(x,y,2,4);step(0,0,0,12)
finally:k.CloseHandle(h)
