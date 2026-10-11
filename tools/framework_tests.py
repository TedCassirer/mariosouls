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
    f.run([sys.executable,f.FRAMEWORK/'tests/collision_test.py'])
    f.run([sys.executable,f.FRAMEWORK/'tests/equipment_assets_test.py'])
    f.BUILD.mkdir(parents=True,exist_ok=True)
    f.compile_cpp(f.FRAMEWORK/'tests/render_test.cpp',f.BUILD/'render-test.exe',extra=['-ld3d9','-ldxguid','-luser32'])
    f.run([f.BUILD/'render-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/class_test.cpp',f.BUILD/'class-test.exe')
    f.run([f.BUILD/'class-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/aim_camera_test.cpp',f.BUILD/'aim-camera-test.exe')
    f.run([f.BUILD/'aim-camera-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/hazard_host_test.cpp',f.BUILD/'hazard-host-test.exe')
    f.run([f.BUILD/'hazard-host-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/core_test.cpp',f.BUILD/'core-test.exe')
    f.run([f.BUILD/'core-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/context_action_test.cpp',f.BUILD/'context-action-test.exe')
    f.run([f.BUILD/'context-action-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/feedback_test.cpp',f.BUILD/'feedback-test.exe')
    f.run([f.BUILD/'feedback-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/motion_test.cpp',f.BUILD/'motion-test.exe')
    f.run([f.BUILD/'motion-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/body_test.cpp',f.BUILD/'body-test.exe')
    f.run([f.BUILD/'body-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/kinematic_test.cpp',f.BUILD/'kinematic-test.exe')
    f.run([f.BUILD/'kinematic-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/contact_test.cpp',f.BUILD/'contact-test.exe')
    f.run([f.BUILD/'contact-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/equipment_test.cpp',f.BUILD/'equipment-test.exe')
    f.run([f.BUILD/'equipment-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/debug_geometry_test.cpp',f.BUILD/'debug-geometry-test.exe')
    f.run([f.BUILD/'debug-geometry-test.exe'])
    for name in ['controller','action_input','native_block']:
        f.compile_cpp(f.FRAMEWORK/f'tests/{name}_test.cpp',f.BUILD/f'{name}-test.exe',extra=['-luser32','-ldxguid'])
        f.run([f.BUILD/f'{name}-test.exe'])
    f.compile_cpp(f.FRAMEWORK/'tests/adapter_test.cpp',f.BUILD/'adapter-test.exe')
    folder,p=f.load_profile(character)
    if p['assets'].get('required') and not asset:raise ValueError('Supply --asset with your owned asset path for this adapter')
    if p['assets'].get('runtime') and not runtime:raise ValueError('Supply --runtime with this character runtime DLL')
    f.run([f.BUILD/'adapter-test.exe',f.BUILD/'characters'/character/'character.dll',asset or '',runtime or ''])
    if p['assets'].get('format')=='SCMESH01':
        f.compile_cpp(f.FRAMEWORK/'tests/mesh_test.cpp',f.BUILD/'mesh-test.exe')
        f.run([f.BUILD/'mesh-test.exe',asset,f.BUILD/'invalid-mesh.scmesh'])
    character_test=folder/'test.cpp'
    if character_test.is_file():
        f.compile_cpp(character_test,f.BUILD/(character+'-test.exe'))
        f.run([f.BUILD/(character+'-test.exe'),f.BUILD/'characters'/character/'character.dll',asset or ''])
    character_equipment_test=folder/'equipment_test.cpp';character_checks=[]
    death_test=folder/'death_test.cpp'
    if death_test.is_file():
        executable=f.BUILD/(character+'-death-test.exe')
        f.compile_cpp(death_test,executable)
        f.run([executable,f.BUILD/'characters'/character/'character.dll',asset or '',runtime or ''])
        character_checks.append(character+' death pose, terminal hold, one-shot sound and new-life reset')
    if character_equipment_test.is_file():
        equipment_test=f.BUILD/(character+'-equipment-test.exe')
        f.compile_cpp(character_equipment_test,equipment_test)
        f.run([equipment_test,f.BUILD/'characters'/character/'character.dll',asset or '',runtime or ''])
        character_checks.append(character+' equipment movement/action parity, rendered poses, sockets and lifecycle')
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
    (f.BUILD/'test-report.json').write_text(json.dumps({'character':character,'checks':['installer transactions and equipment validation','core policies','D3D9 multi-resolution composition, depth occlusion and UI ordering','event collision ownership (3 tests)','equipment asset fitting and bounded prop files (4 tests)','equipment swing lifecycle and swept blade contact','target capsule contact geometry','confirmed-hit feedback','motion mailbox, clock, input latch and ownership lifecycle','native body lifecycle and restoration','controller mapping','native input routes','native block packet identity, exact damage intervals and guard-break observations','real character adapter','generated unrelated adapter','architecture boundaries']+character_checks,'status':'passed','live_game_test':False,'host_sha256':f.sha(f.BUILD/'dinput8.dll'),'adapter_sha256':f.sha(f.BUILD/'characters'/character/'character.dll'),'asset_sha256':f.sha(Path(asset)) if asset else None,'runtime_sha256':f.sha(Path(runtime)) if runtime else None},indent=2))
    print('Framework offline checks passed. This does not replace live-game regression tests.')
