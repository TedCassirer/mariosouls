"""Position only the dedicated test character beside a local Asylum test object."""
import sys,ctypes as c,struct,time
from ctypes import wintypes as w
from pathlib import Path
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
k.WriteProcessMemory.argtypes=k.ReadProcessMemory.argtypes;k.CloseHandle.argtypes=[w.HANDLE]
h=k.OpenProcess(0x1038,False,int(sys.argv[1]));assert h
def read(a,n):
 b=c.create_string_buffer(n);got=c.c_size_t();assert a>0x10000 and k.ReadProcessMemory(h,a,b,n,c.byref(got)) and got.value==n;return b.raw
def u(a):return struct.unpack('<I',read(a,4))[0]
def put(a,b):
 got=c.c_size_t();assert k.WriteProcessMemory(h,a,b,len(b),c.byref(got)) and got.value==len(b)
try:
 stats=u(u(0x1378700)+8);assert read(stats+0xa0,10)=='test\0'.encode('utf-16-le')
 world=u(0x137e204);assert read(world+0xa12,2)==bytes([1,18])
 p=u(u(u(0x137dc70)+4));m=u(p+0x28);pos=u(m+0x1c)
 assert u(p+0x3c4)&0x300==0,'Deactivate Mario first'
 target=list(map(float,sys.argv[2:5]));assert -60<target[0]<100 and 180<target[1]<230 and -100<target[2]<100
 heading=float(sys.argv[5]);xyz=struct.pack('<fff',*target)
 for _ in range(10):
  assert u(p+0x28)==m
  put(m+0xd0,xyz);put(m+0xe4,struct.pack('<f',heading));put(m+0xc8,b'\x01');time.sleep(1/30)
 print('Positioned test at',struct.unpack('<fff',read(pos+0x10,12)))
finally:k.CloseHandle(h)
