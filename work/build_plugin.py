from pathlib import Path
import subprocess,sys
w=Path(__file__).resolve().parent
zig=w/'tools/zig-windows-x86_64-0.13.0/zig.exe'
o=w/'build';objs=[]
for path in [w/'minhook/src/buffer.c',w/'minhook/src/hook.c',w/'minhook/src/trampoline.c',w/'minhook/src/hde/hde32.c']:
 obj=o/(path.stem+'.o');subprocess.run([str(zig),'cc','-target','x86-windows-gnu','-O2','-c',str(path),'-o',str(obj)],check=True);objs.append(str(obj))
args=[str(zig),'c++','-target','x86-windows-gnu','-std=c++17','-O2','-shared','-static-libstdc++',str(w/'mariosouls.cpp'),str(w/'mariosouls.def'),*objs,'-ld3d9','-ldxguid','-luser32','-lwinmm','-o',str(o/'dinput8.dll')]
with (o/'plugin-build.log').open('w') as log:r=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT)
print((o/'plugin-build.log').read_text()[-8000:] if r.returncode else 'Built MarioSouls proxy DLL')
sys.exit(r.returncode)
