"""Recover only the named test character from an observed fall below the Asylum."""
import ctypes as c,struct,sys,time
from ctypes import wintypes as w
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
k.WriteProcessMemory.argtypes=k.ReadProcessMemory.argtypes
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
 world=u(0x137e204);assert read(world+0xa12,2)==bytes([1,18])
 p=u(u(u(0x137dc70)+4));m=u(p+0x28);pos=u(m+0x1c)
 before=struct.unpack('<fff',read(pos+0x10,12));assert before[1]<100,before
 xyz=struct.pack('<fff',-12.699,189.8,22.039)
 for _ in range(30):
  assert u(p+0x28)==m
  put(m+0xc4,struct.pack('<I',u(m+0xc4)&~0x10))
  put(m+0xd0,xyz);put(pos+0x10,xyz);put(m+0xc8,b'\x01');time.sleep(1/30)
 print('Recovered test from',before,'to',struct.unpack('<fff',read(pos+0x10,12)))
finally:k.CloseHandle(h)
