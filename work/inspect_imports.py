import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent/'python-deps'))
import pefile
p=pefile.PE(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\DARKSOULS.exe')
for dll in p.DIRECTORY_ENTRY_IMPORT:
 for i in dll.imports:
  if i.name and any(x in i.name.lower() for x in [b'time',b'tick',b'counter',b'key',b'input',b'message']):print(dll.dll.decode(),i.name.decode(),hex(i.address))
