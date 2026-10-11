from pathlib import Path
import os,sys
os.environ['PATH']=r'C:\Users\Ted\AppData\Local\universal-modder\ffmpeg\bin'+os.pathsep+os.environ['PATH']
from um.video import compile_edl,contact,probe
p=Path(__file__).parent
preview='--preview' in sys.argv
out=p/'preview.mp4' if preview else p.parents[1]/'outputs'/'MarioSouls - Reddit Trailer.mp4'
compile_edl(p/'edit.json',out,preview=preview)
contact(out,p/('preview-sheet.png' if preview else 'final-sheet.png'),every=1,cols=5,width=320)
print(probe(out),flush=True)
