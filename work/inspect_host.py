import ctypes as c
from ctypes import wintypes as w
from pathlib import Path
import json, re, struct, sys
import pefile

GAME=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
PATTERNS={
 'map':('8B 48 04 8B 11 50 8B 42 34 FF D0 C7 46 2C 00 00 00 00 C3 8B 0D ? ? ? ? 8B 89 50 0B 00 00 32 C0 83 79 28 00',0x15,[0]),
 'stats':('A1 ? ? ? ? 8B 40 34 53 32 DB 85 C0',1,[0,8]),
 'player':('8B 15 ? ? ? ? F3 0F 10 44 24 30 52',2,[0,4,0]),
 'camera':('D9 45 08 A1 ? ? ? ? 51 D9 1C 24 50',4,[0,0x3c,0x60]),
 'world':('8B 54 24 10 8B C8 F7 D9 39 8A B8 0E 00 00 B3 01 0F 95 C2 8B 0D ? ? ? ? 80 B9 A5 0B 00 00 00',0x15,[0]),
}
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)];k.ReadProcessMemory.restype=w.BOOL
k.CloseHandle.argtypes=[w.HANDLE]
pid=int(sys.argv[1]);handle=k.OpenProcess(0x1010,False,pid)
if not handle:raise c.WinError(c.get_last_error())
def read(addr,size):
 b=c.create_string_buffer(size);n=c.c_size_t()
 if not k.ReadProcessMemory(handle,addr,b,size,c.byref(n)) or n.value!=size:raise ValueError(f'Cannot read {addr:x}:{size}')
 return b.raw
def u32(addr):return struct.unpack('<I',read(addr,4))[0]
def chain(ptr,offs):
 for off in offs:
  if ptr==0:return 0
  ptr=u32(ptr+off)
 return ptr
pe=pefile.PE(str(GAME/'DARKSOULS.exe'));base=pe.OPTIONAL_HEADER.ImageBase
sec=pe.sections[0];addr=base+sec.VirtualAddress;data=read(addr,sec.Misc_VirtualSize)
report={'pid':pid,'image_base':hex(base),'pointers':{}}
for name,(pattern,imm,offs) in PATTERNS.items():
 rx=b''.join(b'.' if x=='?' else re.escape(bytes([int(x,16)])) for x in pattern.split())
 hits=list(re.finditer(rx,data,re.DOTALL))
 row={'matches':len(hits)}
 if len(hits)==1:
  slot=struct.unpack_from('<I',data,hits[0].start()+imm)[0]
  ptr=chain(slot,offs);row.update(slot=hex(slot),resolved=hex(ptr))
  if ptr and name=='map':row['world_area']=list(read(ptr+0xa12,2)[::-1])
  if ptr and name=='player':
   row['hp']=u32(ptr+0x2d4); row['flags1']=hex(u32(ptr+0x1fc));row['flags2']=hex(u32(ptr+0x3c4))
   mp=u32(ptr+0x28);pos=u32(mp+0x1c) if mp else 0
   row['mapdata']=hex(mp);row['posdata']=hex(pos)
   if pos:row['position']=struct.unpack('<fff',read(pos+0x10,12))
   Path(__file__).with_name('player.bin').write_bytes(read(ptr,0x700))
   root=u32(slot);begin=u32(root+4);end=u32(root+8)
   row['array']=[hex(begin),hex(end)]
   if 0<end-begin<65536:
    enemies=[]
    for address in range(begin,end,4):
     try:
      npc=u32(address)
      if not npc or npc==ptr:continue
      hp=u32(npc+0x2d4);team=u32(npc+0x74)
      if hp==0 or hp>100000:continue
      pos=chain(npc+0x28,[0,0x1c]);xyz=struct.unpack('<fff',read(pos+0x10,12))
      enemies.append({'ptr':hex(npc),'id':u32(npc+0x208),'team':team,'hp':hp,'position':xyz,'model':read(npc+0x38,20).decode('utf-16-le').split('\0')[0]})
     except (ValueError,UnicodeError):continue
    row['characters']=enemies
  if ptr and name=='camera':
   row['position']=struct.unpack('<fff',read(ptr+0x100,12));row['rotation']=struct.unpack('<fff',read(ptr+0x140,12))
   Path(__file__).with_name('camera.bin').write_bytes(read(ptr,0x300))
  if ptr and name=='stats':
   raw=read(ptr+0xA0,40)
   row['name']=raw.decode('utf-16-le',errors='replace').split('\0')[0]
   row['name_hex']=raw.hex()
   row['level']=u32(ptr+0x88)
 report['pointers'][name]=row
k.CloseHandle(handle)
print(json.dumps(report,indent=2));Path(__file__).with_name('host-inspection.json').write_text(json.dumps(report,indent=2))
