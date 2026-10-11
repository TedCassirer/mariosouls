"""Install the tested private runtime and produce a hash-checked restore manifest."""
from pathlib import Path
import shutil,hashlib,json,configparser
w=Path(__file__).resolve().parent
game=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
dest=game/'MarioSouls';dest.mkdir(exist_ok=True)
for d in ['collision','backup','tools','source']:(dest/d).mkdir(exist_ok=True)
shutil.copy2(w/'build/sm64.dll',dest/'sm64.dll')
shutil.copy2(w/'build/dinput8.dll',game/'dinput8.dll')
for p in (w/'collision').glob('*.bin'):shutil.copy2(p,dest/'collision'/p.name)
shutil.copy2(w/'frpg_title.original.wmv',dest/'backup/frpg_title.wmv')
for name in ['fast_test_load.py','continue_small.png','title-long_small.png','load-slot_small.png','slot-test_small.png','quit-warning_small.png','autosave-notice_small.png']:
 shutil.copy2(w/name,dest/'tools'/name)
for name in ['mariosouls.cpp','controller_input.h','startup.h','startup_hash.h','mariosouls.def','build_plugin.py','build_sm64.py','export_collision.py','ds_archive.py']:
 shutil.copy2(w/name,dest/'source'/name)
shutil.copy2(w/'minhook/LICENSE.txt',dest/'source/MinHook-LICENSE.txt')
shutil.copy2(w/'libsm64/LICENSE.md',dest/'source/libsm64-LICENSE.txt')
cfg=configparser.ConfigParser();cfg.optionxform=str;cfg.read(game/'mariosouls.ini')
cfg['MarioSouls'].update(Library=str(dest/'sm64.dll'),Collision=str(dest/'collision/m18_01_00_00.bin'),RenderMode='2',HideNative='1',DepthTest='1',EnableCombat='1',SkipIntro='1')
cfg['Test'].update(Serial=str(int(cfg['Test']['Serial'])+1),Mode='2',Frames='0',Buttons='0',X='0',Y='0')
with (game/'mariosouls.ini').open('w') as f:cfg.write(f)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
manifest={'version':'0.1-experimental','game':str(game),'proxy_sha256':sha(game/'dinput8.dll'),'short_title_sha256':sha(game/'movWW/frpg_title.wmv'),'original_title_sha256':sha(dest/'backup/frpg_title.wmv'),'collision_maps':[p.stem for p in (dest/'collision').glob('*.bin')],'rom_path':cfg['MarioSouls']['ROM']}
(dest/'install.json').write_text(json.dumps(manifest,indent=2))
(w/'prototype-install.json').write_text(json.dumps(manifest,indent=2))
print('Installed standalone runtime:',dest)
print('Collision maps:',len(manifest['collision_maps']))
