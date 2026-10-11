"""Read-only disassembly of the owner's PTDE executable; no process writes."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent/'python-deps'))
import pefile,capstone
pe=pefile.PE(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\DARKSOULS.exe')
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
for arg in sys.argv[1:]:
    bounds=arg.split(':');address=int(bounds[0],16)
    print(f'Function {address:08x}')
    data=pe.get_data(address-pe.OPTIONAL_HEADER.ImageBase,int(bounds[1],16)-address if len(bounds)>1 else 512)
    for i in cs.disasm(data,address):
        print(f'{i.address:08x} {i.mnemonic:8s} {i.op_str}')
        if len(bounds)==1 and i.mnemonic.startswith('ret'):break
