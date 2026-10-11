from pathlib import Path
import os,sys
os.environ['PATH']=r'C:\Users\Ted\AppData\Local\universal-modder\ffmpeg\bin'+os.pathsep+os.environ['PATH']
from um.video import mux,contact,probe
root=Path(__file__).parent
for name in sys.argv[1:]:
 p=root/name
 if not p.with_suffix('.mp4').exists():mux(p.with_suffix('.mkv'),p.with_suffix('.audio.raw'),p.with_suffix('.json'),p.with_suffix('.mp4'))
 contact(p.with_suffix('.mp4'),root/(name+'-sheet.png'),every=1,cols=5,width=320)
 print(probe(p.with_suffix('.mp4')),flush=True)
