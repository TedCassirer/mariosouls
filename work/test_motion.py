import ctypes as c,struct,time,json,threading,sys
from pathlib import Path
sys.path.insert(0,r'C:\Users\Ted\.codex\plugins\cache\universal-modder\universal-modder\0.2.0')
from um.win import Drive
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[c.c_uint32,c.c_bool,c.c_uint32];k.OpenProcess.restype=c.c_void_p
k.ReadProcessMemory.argtypes=[c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_void_p]
k.CloseHandle.argtypes=[c.c_void_p]
handle=k.OpenProcess(0x1010,False,int(sys.argv[1]))
def read(a,n):
 b=c.create_string_buffer(n)
 if not k.ReadProcessMemory(handle,a,b,n,None):raise RuntimeError('read failed')
 return b.raw
def u(a):return struct.unpack('<I',read(a,4))[0]
player=u(u(u(0x137dc70)+4));pos=u(u(player+0x28)+0x1c)
rows=[];stop=False
user=c.WinDLL('user32');user.GetForegroundWindow.restype=c.c_void_p
user.GetWindowThreadProcessId.argtypes=[c.c_void_p,c.POINTER(c.c_uint32)]
def poll():
 start=time.perf_counter()
 while not stop:
  fg=c.c_uint32();user.GetWindowThreadProcessId(user.GetForegroundWindow(),c.byref(fg))
  rows.append([round(time.perf_counter()-start,4),*struct.unpack('<3f',read(pos+0x10,12)),bool(user.GetAsyncKeyState(0x20)&0x8000),fg.value]);time.sleep(.02)
d=Drive('DARKSOULS');d.focus();d.cmd('scanmode off')
t=threading.Thread(target=poll);t.start()
try:
 time.sleep(.25)
 print(d.cmd('hold 0x20 300'))
 time.sleep(2)
finally:
 stop=True;t.join();d.close();k.CloseHandle(handle)
Path(__file__).with_name('motion-test.json').write_text(json.dumps(rows))
ys=[r[2] for r in rows]
print(json.dumps({'samples':len(rows),'start_y':ys[0],'max_y':max(ys),'end_y':ys[-1],'jump_metres':max(ys)-ys[0],'key_samples':sum(r[4] for r in rows),'foreground':sorted(set(r[5] for r in rows))}))
