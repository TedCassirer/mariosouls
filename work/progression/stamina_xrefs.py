from pathlib import Path
import sys,struct
sys.path.insert(0,str(Path('work/python-deps').resolve()))
import pefile,capstone
p=pefile.PE(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\DARKSOULS.exe')
for sec in p.sections:
 if not sec.Characteristics&0x20000000:continue
 b=sec.get_data();base=p.OPTIONAL_HEADER.ImageBase+sec.VirtualAddress
 for i in range(len(b)-5):
  if b[i]==0xe8 and base+i+5+struct.unpack_from('<i',b,i+1)[0]==0xe67960:print(hex(base+i))
