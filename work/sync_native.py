"""Private test: propagate Mario's existing position through the native warp request."""
import ctypes as c,struct,json,sys,time
from ctypes import wintypes as w
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
k.WriteProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
k.CloseHandle.argtypes=[w.HANDLE]
h=k.OpenProcess(0x1038,False,int(sys.argv[1]))
def read(a,n):
 b=c.create_string_buffer(n);got=c.c_size_t();assert k.ReadProcessMemory(h,a,b,n,c.byref(got)) and got.value==n;return b.raw
def u(a):return struct.unpack('<I',read(a,4))[0]
def put(a,b):
 got=c.c_size_t();assert k.WriteProcessMemory(h,a,b,len(b),c.byref(got)) and got.value==len(b)
try:
 stats=u(u(0x1378700)+8)
 assert read(stats+0xa0,40).decode('utf-16-le',errors='replace').split('\0')[0]=='test'
 p=u(u(u(0x137dc70)+4));print("Player entity ID",u(p+0x208));assert p>0x10000
 m=u(p+0x28);pos=u(m+0x1c)
 for _ in range(int(sys.argv[2]) if len(sys.argv)>2 else 1):
  xyz=read(pos+0x10,12);put(m+0xd0,xyz);put(m+0xe4,read(pos+4,4));put(m+0xc8,b'\x01');time.sleep(1/30)
 print('Native warp synchronized',struct.unpack('<fff',xyz))
finally:k.CloseHandle(h)
