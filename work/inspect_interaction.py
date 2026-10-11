"""Read-only PTDE interaction/animation observations from the named test character."""
import ctypes as c,struct,sys,time,json
from pathlib import Path
from ctypes import wintypes as w
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
k.CloseHandle.argtypes=[w.HANDLE]
h=k.OpenProcess(0x1010,False,int(sys.argv[1]));assert h
def read(a,n):
 b=c.create_string_buffer(n);got=c.c_size_t()
 if a<0x10000 or not k.ReadProcessMemory(h,a,b,n,c.byref(got)) or got.value!=n:raise ValueError(hex(a))
 return b.raw
def u(a):return struct.unpack('<I',read(a,4))[0]
def f(a):return struct.unpack('<f',read(a,4))[0]
def chain(a,*offs):
 for off in offs:a=u(a+off)
 return a
def snapshot():
 stats=chain(0x1378700,0,8);assert read(stats+0xa0,40).decode('utf-16-le',errors='replace').split('\0')[0]=='test'
 p=chain(0x137dc70,0,4,0);m=u(p+0x28);pos=u(m+0x1c);anim=u(m+0x14);node=u(anim+0xc);begin=u(node+0x10);count=u(node+0x14)
 menu=u(0x13786d0)
 out={'pos':struct.unpack('<fff',read(pos+0x10,12)),'heading':f(pos+4),'flags1':hex(u(p+0x1fc)),'flags2':hex(u(p+0x3c4)),'mapflags':hex(u(m+0xc4)),'anim':hex(anim),'node':hex(node),'count':count,'camp':u(menu+0x210),'menu':{hex(i):u(menu+i) for i in [0x10c,0x60,0x208,0x210]},'instances':[]}
 if count<32:
  for i in range(count):
   a=u(begin+i*4);raw=read(a,0x80)
   out['instances'].append({'ptr':hex(a),'time':f(a+8),'weight':f(a+0x3c),'values':{hex(j):hex(struct.unpack_from('<I',raw,j)[0]) for j in range(0,0x80,4)}})
 action=u(p+0x148);tracks=u(m+0x18)
 out['event_action']=[u(action+i) for i in range(0,0x18,4)]
 out['forced']=[u(p+i) for i in range(0xfc,0x10c,4)]
 out['native_anim']=u(u(p+0x10)+4)
 out['moveflags']=hex(u(m+0xc0))
 out['tracks']=[[i,u(tracks+i*0x9c),u(tracks+i*0x9c+4)] for i in range(29)] if tracks else []
 out['menu_active']={hex(i):u(menu+i) for i in range(0x24,0x124,4) if u(menu+i)}
 if '--detail' not in sys.argv:out.pop('instances')
 return out
try:
 if len(sys.argv)<3 or '--detail' in sys.argv:print(json.dumps(snapshot(),indent=2))
 else:
  end=time.monotonic()+float(sys.argv[2]);last=None
  with Path(sys.argv[3]).open('w') as out:
   while time.monotonic()<end:
    try:s=snapshot();out.write(json.dumps({'time':time.monotonic(),**s})+'\n');out.flush()
    except (ValueError,AssertionError):pass
    time.sleep(.1)
finally:k.CloseHandle(h)
