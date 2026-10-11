from pathlib import Path
import subprocess
import sys

work = Path(__file__).resolve().parent
repo = work / 'libsm64'
zig = work / 'tools/zig-windows-x86_64-0.13.0/zig.exe'
out = work / 'build'
out.mkdir(exist_ok=True)
dirs = ['src', 'src/decomp', 'src/decomp/engine', 'src/decomp/include/PR',
        'src/decomp/game', 'src/decomp/pc', 'src/decomp/pc/audio',
        'src/decomp/mario', 'src/decomp/tools', 'src/decomp/audio']
sources = [str(p) for d in dirs for p in sorted((repo / d).glob('*.c'))]
args = [str(zig), 'cc', '-target', 'x86-windows-gnu', '-std=gnu11',
        '-O2', '-fno-strict-aliasing', '-DSM64_LIB_EXPORT', '-DGBI_FLOATS',
        '-DVERSION_US', '-DNO_SEGMENTED_MEMORY', '-Wno-int-conversion',
        '-Wno-incompatible-pointer-types', '-I', str(repo / 'src/decomp/include'),
        '-shared', *sources, '-o', str(out / 'sm64.dll'), '-lm']
print(f'Compiling {len(sources)} C files for the 32-bit host.', flush=True)
with (out / 'sm64-build.log').open('w', encoding='utf-8') as log:
    result = subprocess.run(args, stdout=log, stderr=subprocess.STDOUT)
if result.returncode:
    print((out / 'sm64-build.log').read_text()[-10000:])
else:
    print('Built: ' + str(out / 'sm64.dll'))
sys.exit(result.returncode)
