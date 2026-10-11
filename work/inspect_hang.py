"""Sample one known game thread, always resuming it immediately after context capture."""
import ctypes as c,struct,sys,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent/'python-deps'))
import capstone
k=c.WinDLL('kernel32',use_last_error=True)
for name,args,ret in [('OpenProcess',[c.c_uint32,c.c_bool,c.c_uint32],c.c_void_p),('OpenThread',[c.c_uint32,c.c_bool,c.c_uint32],c.c_void_p),('SuspendThread',[c.c_void_p],c.c_uint32),('ResumeThread',[c.c_void_p],c.c_uint32),('CloseHandle',[c.c_void_p],c.c_bool),('Wow64GetThreadContext',[c.c_void_p,c.c_void_p],c.c_bool),('ReadProcessMemory',[c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_void_p],c.c_bool)]:
 fn=getattr(k,name);fn.argtypes=args;fn.restype=ret
p=k.OpenProcess(0x1010,False,int(sys.argv[1]));t=k.OpenThread(0x4a,False,int(sys.argv[2]));ctx=c.create_string_buffer(716);struct.pack_into('<I',ctx,0,0x10007)
if not p or not t:raise c.WinError(c.get_last_error())
prior=k.SuspendThread(t)
if prior==0xffffffff:raise c.WinError(c.get_last_error())
try:
 if not k.Wow64GetThreadContext(t,ctx):raise c.WinError(c.get_last_error())
finally:k.ResumeThread(t);k.CloseHandle(t)
def read(a,n):
 b=c.create_string_buffer(n)
 if not k.ReadProcessMemory(p,a,b,n,None):return b''
 return b.raw
regs=dict(zip(['edi','esi','ebx','edx','ecx','eax','ebp','eip','cs','eflags','esp','ss'],struct.unpack_from('<12I',ctx,156)))
print({key:hex(val) for key,val in regs.items()})
mods=json.loads(Path(__file__).with_name('hang-modules.json').read_text(encoding='utf-8-sig'))
def symbol(a):
 for m in mods:
  if m['start']<=a<m['end']:return f"{m['name']}+{a-m['start']:x}"
 return ''
print('EIP',symbol(regs['eip']))
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
for i in cs.disasm(read(regs['eip'],96),regs['eip']):print(hex(i.address),i.mnemonic,i.op_str)
stack=read(regs['esp'],1024)
for off in range(0,len(stack),4):
 a=struct.unpack_from('<I',stack,off)[0];s=symbol(a)
 if s:print(f'stack+{off:x}: {a:08x} {s}')
k.CloseHandle(p)
