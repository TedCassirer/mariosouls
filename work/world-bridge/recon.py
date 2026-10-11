from pathlib import Path
import sys,struct,re
w=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(w/'python-deps'))
import pefile,capstone
p=pefile.PE(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\DARKSOULS.exe')
c=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);c.skipdata=True
for s in p.sections:
 if not s.Characteristics&0x20000000:continue
 for i in c.disasm(s.get_data(),p.OPTIONAL_HEADER.ImageBase+s.VirtualAddress):
  if len(sys.argv)>1:
   if '0x2a]' in i.op_str and 'esp' not in i.op_str and 'ebp' not in i.op_str:print(hex(i.address),i.mnemonic,i.op_str)
  elif i.mnemonic in ('test','cmp') and ('0xc8]' in i.op_str or ('0xc4]' in i.op_str and i.op_str.endswith(', 0x10'))):print(hex(i.address),i.mnemonic,i.op_str)
