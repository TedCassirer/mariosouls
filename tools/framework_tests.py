"""Offline framework checks; no input or writes to the running game."""
import json
import shutil
import subprocess
import sys
from pathlib import Path
from unittest.mock import patch
import framework as f


def test(character,asset,runtime):
    (f.BUILD/'test-report.json').unlink(missing_ok=True)
    f.architecture_check()
    f.run([sys.executable,f.ROOT/'tools/test_framework_install.py'])
    f.BUILD.mkdir(parents=True,exist_ok=True)
    f.compile_cpp(f.FRAMEWORK/'tests/core_test.cpp',f.BUILD/'core-test.exe')
    f.run([f.BUILD/'core-test.exe'])
    for name in ['controller','action_input']:
        f.compile_cpp(f.FRAMEWORK/f'tests/{name}_test.cpp',f.BUILD/f'{name}-test.exe',extra=['-luser32','-ldxguid'])
        f.run([f.BUILD/f'{name}-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/adapter_test.cpp',f.BUILD/'adapter-test.exe')
    folder,p=f.load_profile(character)
    if p['assets'].get('required') and not asset:raise ValueError('Supply --asset with your owned asset path for this adapter')
    if p['assets'].get('runtime') and not runtime:raise ValueError('Supply --runtime with this character runtime DLL')
    f.run([f.BUILD/'adapter-test.exe',f.BUILD/'characters'/character/'character.dll',asset or '',runtime or ''])
    # Generate a second, unrelated adapter in an isolated scaffold tree, then
    # load it through precisely the same SDK runner without compiling host edits.
    sandbox=f.BUILD/'scaffold-test';sandbox.mkdir(exist_ok=True)
    (sandbox/'templates').mkdir(exist_ok=True)
    shutil.copy2(f.FRAMEWORK/'templates/adapter.cpp.in',sandbox/'templates/adapter.cpp.in')
    generated=sandbox/'characters/sdk_probe'
    if generated.exists():
        if not generated.resolve().is_relative_to(sandbox.resolve()):raise RuntimeError('Unexpected scaffold location')
        shutil.rmtree(generated)
    with patch.object(f,'FRAMEWORK',sandbox):f.scaffold('sdk_probe','SDK conformance fixture')
    profile=json.loads((generated/'character.json').read_text());f.profile_header(profile,generated/'profile.generated.h')
    f.compile_cpp(generated/'adapter.cpp',generated/'character.dll',[generated],['-shared'])
    f.run([f.BUILD/'adapter-test.exe',generated/'character.dll'])
    (f.BUILD/'test-report.json').write_text(json.dumps({'character':character,'checks':['installer transactions (5 tests)','core policies','controller mapping','native input routes','real character adapter','generated unrelated adapter','architecture boundaries'],'status':'passed','live_game_test':False,'host_sha256':f.sha(f.BUILD/'dinput8.dll'),'adapter_sha256':f.sha(f.BUILD/'characters'/character/'character.dll'),'asset_sha256':f.sha(Path(asset)) if asset else None,'runtime_sha256':f.sha(Path(runtime)) if runtime else None},indent=2))
    print('Framework offline checks passed. This does not replace live-game regression tests.')
