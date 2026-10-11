from pathlib import Path
import sys
sys.path.insert(0,str(Path('work/python-deps').resolve()))
import pefile,capstone
p=pefile.PE(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\DARKSOULS.exe');c=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);c.skipdata=True
for sec in p.sections:
 if sec.Characteristics&0x20000000:
  for i in c.disasm(sec.get_data(),p.OPTIONAL_HEADER.ImageBase+sec.VirtualAddress):
   if '0x2e4]' in i.op_str and i.mnemonic in ('mov','sub','add','movss','fstp'):
    print(hex(i.address),i.mnemonic,i.op_str)
