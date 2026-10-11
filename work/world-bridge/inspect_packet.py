import struct,json
from pathlib import Path
g=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA\MarioSouls')
b=(g/'object-hit-0.bin').read_bytes()
for i in range(0,0x190,4):print(f'{i:03x}',hex(struct.unpack_from('<I',b,i)[0]),f'{struct.unpack_from("<f",b,i)[0]:.5g}')
a=json.load(open('outputs/Combat harness/20261011-002707/native-object.json'))
print(*[x for x in a['before'] if x['hp']>0 and abs(x['position'][0]+3.6)<2 and -16<x['position'][2]<-7],sep='\n')
