"""One-time capture of the 0.7 checkpoint. Does not write to the game installation."""
from pathlib import Path
import datetime, hashlib, json, shutil, subprocess, zipfile

root=Path(__file__).resolve().parents[1]
assert not (root/'work/checkpoints/mariosouls-v0.7-runtime.zip').exists(),'Checkpoint already captured; do not overwrite its manifests or patches'
game=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
git=lambda p,*a:subprocess.run(['git','-C',str(p),*a],capture_output=True,check=True).stdout
cp=root/'checkpoints';cp.mkdir(exist_ok=True)
patch=root/'patches/libsm64';patch.mkdir(parents=True,exist_ok=True)
lib=root/'work/libsm64'
(root/'patches/libsm64.patch').write_bytes(git(lib,'diff','--binary','--full-index','HEAD'))
shutil.copy2(lib/'src/host_pose.h',patch/'host_pose.h')
changed=git(lib,'diff','--name-only','HEAD').decode().splitlines()+['src/host_pose.h']
lock={'build_dependencies':[
 {'path':'work/libsm64','url':'https://github.com/libsm64/libsm64.git','commit':git(lib,'rev-parse','HEAD').decode().strip(),
  'patch':'patches/libsm64.patch','overlay':{'patches/libsm64/host_pose.h':'src/host_pose.h'},
  'modified_files':{name:sha(lib/name) for name in changed}},
 {'path':'work/minhook','url':'https://github.com/TsudaKageyu/minhook.git','commit':git(root/'work/minhook','rev-parse','HEAD').decode().strip()}],
 'compiler':{'version':'Zig 0.13.0 Windows x86_64','target':'x86-windows-gnu','path':'work/tools/zig-windows-x86_64-0.13.0/zig.exe','sha256':sha(root/'work/tools/zig-windows-x86_64-0.13.0/zig.exe'),
 'local_archive':'work/tools/zig-0.13.0.zip','archive_sha256':sha(root/'work/tools/zig-0.13.0.zip')},
 'testing':{'python':'3.12','universal_modder':'0.2.0','local_python_deps':'work/python-deps','optional_packages':['pefile','numpy','Pillow','scipy','soulstruct']}}
(root/'dependencies.lock.json').write_text(json.dumps(lock,indent=2)+'\n')

archive=root/'work/checkpoints/mariosouls-v0.7-runtime.zip';archive.parent.mkdir(exist_ok=True)
assert not archive.exists(),'Keep existing checkpoint immutable'
files=[game/name for name in ['dinput8.dll','mariosouls.ini','MarioSouls/sm64.dll','MarioSouls/harness.ini','MarioSouls/install.json','movWW/frpg_title.wmv','MarioSouls/backup/frpg_title.wmv']]
for directory in ['MarioSouls/collision','MarioSouls/source']:
 files.extend(p for p in (game/directory).rglob('*') if p.is_file())
files.extend(p for p in (game/'MarioSouls/tools').iterdir() if p.is_file() and (p.suffix=='.py' or p.name.endswith('_small.png')))
installed=json.loads((game/'MarioSouls/install.json').read_text())
assert sha(game/'dinput8.dll')==installed['proxy_sha256']
assert sha(game/'MarioSouls/sm64.dll')==installed['sm64_sha256']
manifest={'name':'MarioSouls 0.7 contextual interactions checkpoint','tag':'checkpoint-2026-10-11-mariosouls-v0.7','created':datetime.datetime.now().astimezone().isoformat(),
 'game_directory':str(game),'game_exe_sha256':sha(game/'DARKSOULS.exe'),'archive':archive.relative_to(root).as_posix(),
 'files':{},'save_files_included':False,'rom_included':False,'notes':'Private local runtime cache. Keep it with the repository; it is deliberately excluded from Git.'}
with zipfile.ZipFile(archive,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for p in sorted(files):
  name=p.relative_to(game).as_posix();data=p.read_bytes()
  manifest['files'][name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
  z.writestr(name,data)
manifest['archive_sha256']=sha(archive)
(cp/'mariosouls-v0.7.json').write_text(json.dumps(manifest,indent=2)+'\n')
shutil.copy2(game/'mariosouls.ini',cp/'mariosouls-v0.7.ini')
shutil.copy2(game/'MarioSouls/install.json',cp/'installed-v0.7.json')

# An allowlist keeps experimental assets, dependencies and game-derived data out.
lines=['# Source checkpoint only. Local runtime archives and game-derived data stay private.',
 '/*','!/.gitignore','!/.gitattributes','!/README.md','!/dependencies.lock.json',
 '!/tools/','/tools/*','!/tools/*.py',
 '!/checkpoints/','/checkpoints/*','!/checkpoints/*.json','!/checkpoints/*.ini',
 '!/patches/','/patches/*','!/patches/*.patch','!/patches/libsm64/','/patches/libsm64/*','!/patches/libsm64/host_pose.h',
 '!/work/','/work/*','!/work/*.cpp','!/work/*.c','!/work/*.h','!/work/*.def','!/work/*.py','!/work/MODLOG.md','!/work/MODDING_PLAN.md']
# This one file was extracted from the native game, not authored mod code.
lines+=['/work/asylum.evs.py']
for directory in ['combat-harness','progression','interaction-v2','world-bridge','ui-render','docs','field-notes']:
 lines += [f'!/work/{directory}/',f'/work/{directory}/*']
 for extension in ['py','cpp','md']:
  lines += [f'!/work/{directory}/*.{extension}']
lines+=['!/work/interaction-v2/PLAN.txt','!/outputs/','/outputs/*','!/outputs/*.txt','!/outputs/*.csv','!/outputs/*.ps1','!/outputs/*.cmd',
 '!/outputs/Combat harness/','/outputs/Combat harness/*','!/outputs/Combat harness/*.md','!/outputs/Combat harness/*.ps1','!/outputs/Combat harness/*.cmd','!/outputs/Combat harness/Example punch sequence.json','!/outputs/Combat harness/fixture-baseline.json']
(root/'.gitignore').write_text('\n'.join(lines)+'\n')
print('Captured',len(files),'runtime files,',archive.stat().st_size,'compressed bytes')
print('Source patch includes',len(changed),'modified/new libsm64 files')
