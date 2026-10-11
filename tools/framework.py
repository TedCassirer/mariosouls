"""Souls Character Framework: build, validate, scaffold and install character adapters."""
from pathlib import Path
import argparse
import configparser
import datetime
import hashlib
import json
import math
import os
import re
import shutil
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[1]
FRAMEWORK=ROOT/'framework'
BUILD=ROOT/'work/framework-build'
ZIG=ROOT/'work/tools/zig-windows-x86_64-0.13.0/zig.exe'
GAME=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA')
FLAGS={'automatic':1,'radial':2,'no_objects':4}
CAPS={'audio':1,'item_prop':2,'texture_modulate':4,'alpha_test':8,'equipment':16,'death_pose':32,'melee_families':64,'native_combat':128,'pyromancy':256}
BODY_DEFAULTS=dict(model_scale=1.,radius_cm=0.,height_cm=0.,hurt_radius_cm=0.,hurt_height_cm=0.,fall_damage_scale=1.,fall_damage_max_hp_fraction=1.,nonlethal_falls=False)


def run(args,**kw):
    subprocess.run([str(a) for a in args],check=True,**kw)


def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()


def load_profile(character):
    if not re.fullmatch('[a-z][a-z0-9_]{0,46}',character):raise ValueError('Use a lowercase character identifier')
    folder=FRAMEWORK/'characters'/character
    p=json.loads((folder/'character.json').read_text())
    if p['schema']!=1 or p['id']!=character:raise ValueError('Manifest version/identity mismatch')
    if not re.fullmatch('[a-zA-Z0-9_./-]+',p['source']) or not (folder/p['source']).resolve().is_relative_to(folder.resolve()):raise ValueError('Adapter source must stay inside its character folder')
    if not (folder/p['source']).is_file():raise ValueError('Adapter source is missing')
    if len(p['attacks'])>32 or not 1<=p['max_triangles']<=16384 or len(p['texture'])!=2 or not all(1<=v<=4096 for v in p['texture']):raise ValueError('Profile exceeds SDK bounds')
    body=BODY_DEFAULTS|p.get('body',{})
    if set(body)!=set(BODY_DEFAULTS):raise ValueError('Unknown body setting')
    if not isinstance(body['nonlethal_falls'],bool):raise ValueError('nonlethal_falls must be boolean')
    for key,value in body.items():
        if key!='nonlethal_falls' and (not isinstance(value,(int,float)) or isinstance(value,bool) or not math.isfinite(value)):raise ValueError('Invalid body number: '+key)
    if not .25<=body['model_scale']<=2 or not 0<=body['fall_damage_scale']<=1 or not 0<=body['fall_damage_max_hp_fraction']<=1:raise ValueError('Body scale or fall policy exceeds SDK bounds')
    if any(body[k]!=0 for k in ['radius_cm','height_cm','hurt_radius_cm','hurt_height_cm']):
        if not (10<=body['radius_cm']<=40 and 2*body['radius_cm']<=body['height_cm']<=150 and 10<=body['hurt_radius_cm']<=45 and 2*body['hurt_radius_cm']<=body['hurt_height_cm']<=160):raise ValueError('Body dimensions exceed SDK bounds')
    p['body']=body
    camera=p.get('camera',{'distance_scale':1.0})
    scale=camera.get('distance_scale')
    if set(camera)!={'distance_scale'} or isinstance(scale,bool) or not isinstance(scale,(int,float)) or not math.isfinite(scale) or not 1<=scale<=2:raise ValueError('Invalid camera distance scale (1..2)')
    p['camera']=camera
    names=[]
    for attack in p['attacks']:
        name=attack['name']
        if not re.fullmatch('[a-z][a-z0-9_]{0,30}',name) or name in names:raise ValueError('Attack names must be unique identifiers')
        names.append(name)
        if any(flag not in FLAGS for flag in attack['flags']):raise ValueError('Unknown attack flag')
        knockback=attack.get('knockback_cm',0)
        if isinstance(knockback,bool) or not isinstance(knockback,(int,float)) or not math.isfinite(knockback) or not 0<=knockback<=200:raise ValueError('Attack knockback must be 0..200 cm')
    if any(cap not in CAPS for cap in p['capabilities']):raise ValueError('Unknown capability')
    runtime=p.get('assets',{}).get('runtime')
    if runtime and not re.fullmatch('[a-zA-Z0-9_-]+\\.dll',runtime):raise ValueError('Runtime must be a DLL filename')
    for obj in [p,*p['attacks']]:
        if any(isinstance(v,(int,float)) and not math.isfinite(v) for v in obj.values()):raise ValueError('Profile numbers must be finite')
    if p.get('automatic_attack') is not None and p['automatic_attack'] not in names:raise ValueError('Automatic attack does not exist')
    return folder,p


def profile_header(p,path):
    # Initializer code is emitted only from validated numeric values and JSON-escaped strings.
    num=lambda x:format(float(x),'.9g')+'f' if '.' in format(float(x),'.9g') or 'e' in format(float(x),'.9g') else format(float(x),'.9g')+'.0f'
    def integer(x):
        if not isinstance(x,int) or isinstance(x,bool) or x<0:raise ValueError('Expected non-negative integer')
        return str(x)
    attacks=[]
    for a in p['attacks']:
        attacks.append('{'+','.join([json.dumps(a['name']),num(a['multiplier']),num(a['poise']),integer(a['stamina_cost']),str(sum(FLAGS[f] for f in a['flags'])),integer(a['active_ticks']),integer(a['max_targets']),num(a['range_cm']),num(a['min_height_cm']),num(a['max_height_cm']),num(a.get('automatic_radius_cm',0)),num(a.get('automatic_min_height_cm',0)),num(a.get('automatic_max_height_cm',0))])+'}')
    names=[a['name'] for a in p['attacks']]
    auto=names.index(p['automatic_attack']) if p.get('automatic_attack') else -1
    body=p.get('body',BODY_DEFAULTS)
    body_code=','.join(num(body[k]) for k in list(BODY_DEFAULTS)[:-1])+','+str(int(body['nonlethal_falls']))
    text='static const SC_Descriptor characterProfile={sizeof(SC_Descriptor),SC_ABI_VERSION,'+','.join([json.dumps(p['id']),json.dumps(p['display_name']),integer(p['max_triangles']),integer(p['texture'][0]),integer(p['texture'][1]),str(sum(CAPS[c] for c in p['capabilities'])),integer(p.get('audio_rate',0)),str(len(attacks)),str(auto),num(p['base_power']),num(p['upgrade_power']),num(p['dexterity_weight'])])+',{'+','.join(attacks)+'},{'+body_code+'}};\n'
    path.write_text(text,encoding='utf-8')
    with path.open('a',encoding='utf-8') as stream:stream.write('static const SC_CameraProfile characterCameraProfile={sizeof(SC_CameraProfile),SC_CAMERA_PROFILE_VERSION,'+num(p.get('camera',{'distance_scale':1.0})['distance_scale'])+'};\n')
    with path.open('a',encoding='utf-8') as stream:stream.write('static const SC_ImpactProfile characterImpactProfile={sizeof(SC_ImpactProfile),SC_IMPACT_PROFILE_VERSION,'+str(len(attacks))+',1200,80,{'+','.join(num(a.get('knockback_cm',0)) for a in p['attacks'])+'}};\n')


def compile_cpp(source,dest,includes=(),extra=()):
    dest.parent.mkdir(parents=True,exist_ok=True)
    args=[ZIG,'c++','-target','x86-windows-gnu','-std=c++17','-O2','-static-libstdc++','-I',FRAMEWORK/'sdk']
    for path in includes:args.extend(['-I',path])
    args.extend([source,*extra,'-o',dest])
    log=dest.with_suffix(dest.suffix+'.build.log')
    with log.open('w') as f:
        result=subprocess.run([str(a) for a in args],stdout=f,stderr=subprocess.STDOUT)
    if result.returncode:
        print(log.read_text()[-14000:]);raise RuntimeError('Build failed: '+str(source))


def build(character,adapter_only=False,with_runtime=False):
    folder,p=load_profile(character);out=BUILD/'characters'/character;out.mkdir(parents=True,exist_ok=True)
    (out/'build.json').unlink(missing_ok=True)
    if with_runtime:
        script=folder/'build_runtime.py'
        if not script.is_file():raise ValueError('This adapter has no runtime build script')
        run([sys.executable,script,'--output',out])
    profile_header(p,out/'profile.generated.h')
    compile_cpp(folder/p['source'],out/'character.dll',[out],['-shared'])
    if not adapter_only:
        objects=[]
        for name in ['buffer','hook','trampoline','hde/hde32']:
            source=ROOT/'work/minhook/src'/f'{name}.c';obj=BUILD/(source.stem+'.o')
            run([ZIG,'cc','-target','x86-windows-gnu','-O2','-c',source,'-o',obj]);objects.append(obj)
        compile_cpp(FRAMEWORK/'hosts/ds1ptde/runtime.cpp',BUILD/'dinput8.dll',extra=['-shared',FRAMEWORK/'hosts/ds1ptde/proxy.def',*objects,'-ld3d9','-ldxguid','-luser32','-lwinmm','-lgdi32'])
    source_paths=sorted(p for base in [FRAMEWORK,ROOT/'tools'] for p in base.rglob('*') if p.is_file() and p.suffix in ['.h','.c','.cpp','.def','.py','.json','.in'])
    receipt={'framework_version':'0.8.3','abi':4,'character':character,'profile':p,'built_at':datetime.datetime.now().astimezone().isoformat(),
        'adapter_sha256':sha(out/'character.dll'),'source_sha256':{f.relative_to(ROOT).as_posix():sha(f) for f in source_paths}}
    if not adapter_only:receipt['host_sha256']=sha(BUILD/'dinput8.dll')
    (out/'build.json').write_text(json.dumps(receipt,indent=2))
    print('Built framework host + '+character if not adapter_only else 'Built adapter '+character)


def scaffold(character,name):
    if not re.fullmatch('[a-z][a-z0-9_]{0,46}',character):raise ValueError('Invalid character id')
    dest=FRAMEWORK/'characters'/character
    if dest.exists():raise ValueError('Character folder already exists; no files replaced')
    dest.mkdir(parents=True)
    p={'schema':1,'id':character,'display_name':name,'status':'scaffold','source':'adapter.cpp','max_triangles':12,'texture':[1,1], 'audio_rate':0,'capabilities':[],'automatic_attack':None,'base_power':100,'upgrade_power':10,'dexterity_weight':.5,'assets':{'required':False,'description':'Procedural conformance placeholder; replace with an owned-asset adapter'},'attacks':[]}
    (dest/'character.json').write_text(json.dumps(p,indent=2)+'\n')
    shutil.copy2(FRAMEWORK/'templates/adapter.cpp.in',dest/'adapter.cpp')
    (dest/'README.md').write_text('Scaffold only. The procedural triangle is an SDK example, not a game-character port.\nSee framework/docs/ADDING_A_CHARACTER.md.\n')
    print('Created scaffold at '+str(dest))


def architecture_check():
    errors=[]
    for base in ['hosts','core','sdk']:
        for f in (FRAMEWORK/base).rglob('*'):
            if f.suffix not in ['.h','.cpp']:continue
            s=f.read_text()
            if re.search(r'\b(?:SM64\w*|sm64_\w+|libsm64)\b',s):errors.append(str(f.relative_to(ROOT))+': guest runtime reference')
            if re.search(r'state\.raw(?:Action|Flags)\s*&',s):errors.append(str(f.relative_to(ROOT))+': opaque guest state used as host logic')
    for f in (FRAMEWORK/'characters').rglob('*.cpp'):
        s=f.read_text()
        if re.search(r'\b(?:WriteProcessMemory|ReadProcessMemory|MH_CreateHook)\b',s):errors.append(str(f.relative_to(ROOT))+': native host access inside adapter')
    if errors:raise RuntimeError('\n'.join(errors))
    print('Architecture boundaries verified')


def doctor(character):
    architecture_check();folder,p=load_profile(character)
    run([sys.executable,ROOT/'tools/dependencies.py'])
    if not ZIG.is_file():raise RuntimeError('Zig toolchain missing')
    asset_lock=folder/'asset_dependencies.json'
    if asset_lock.is_file():
        for name,dep in json.loads(asset_lock.read_text()).items():
            if not isinstance(dep,dict):continue
            if 'exe' in dep:
                tool=ROOT/dep['exe']
                if not tool.is_file() or sha(tool)!=dep['exe_sha256']:raise RuntimeError('Restore pinned asset tool: '+name)
            if 'commit' in dep:
                actual=subprocess.run(['git','-C',str(ROOT/dep['path']),'rev-parse','HEAD'],capture_output=True,text=True,check=True).stdout.strip()
                if actual!=dep['commit']:raise RuntimeError('Asset dependency revision differs: '+name)
        print('Pinned asset tools verified')
    print('Profile valid: '+p['id']+' ('+p['status']+'); '+str(len(p['attacks']))+' attacks')


def main():
    parser=argparse.ArgumentParser(description=__doc__);commands=parser.add_subparsers(dest='command',required=True)
    for name in ['build','doctor','test','install']:
        p=commands.add_parser(name);p.add_argument('--character',default='mario64')
        if name=='build':
            p.add_argument('--adapter-only',action='store_true');p.add_argument('--with-runtime',action='store_true')
        if name=='test':p.add_argument('--asset');p.add_argument('--runtime')
        if name=='install':p.add_argument('--asset');p.add_argument('--runtime');p.add_argument('--check-only',action='store_true');p.add_argument('--collision-directory');p.add_argument('--equipment-directory')
    p=commands.add_parser('new-character');p.add_argument('id');p.add_argument('--name',required=True)
    p=commands.add_parser('prepare-assets');p.add_argument('--character',required=True);p.add_argument('--input',required=True);p.add_argument('--output',required=True)
    p=commands.add_parser('prepare-collision');p.add_argument('--map',required=True);p.add_argument('--output',required=True)
    p=commands.add_parser('prepare-equipment');p.add_argument('--game',default=str(GAME));p.add_argument('--output',required=True)
    p=commands.add_parser('preview-equipment');p.add_argument('--actions',choices=['melee','casting'],default='melee');p.add_argument('--character',default='mario64');p.add_argument('--asset',required=True);p.add_argument('--runtime',required=True);p.add_argument('--equipment-directory',required=True);p.add_argument('--output',required=True)
    args=parser.parse_args()
    if args.command=='new-character':scaffold(args.id,args.name)
    elif args.command=='prepare-collision':run([sys.executable,FRAMEWORK/'hosts/ds1ptde/prepare_collision.py','--game',GAME,'--map',args.map,'--output',args.output])
    elif args.command=='prepare-equipment':run([sys.executable,FRAMEWORK/'hosts/ds1ptde/prepare_equipment.py','--game',args.game,'--output',args.output])
    elif args.command=='preview-equipment':
        folder,profile=load_profile(args.character)
        source=folder/('casting_preview.cpp' if args.actions=='casting' else 'equipment_preview.cpp')
        if not source.is_file():raise ValueError('This adapter has no equipment preview fixture')
        output=Path(args.output).resolve();output.mkdir(parents=True,exist_ok=False)
        executable=BUILD/(args.character+'-equipment-preview.exe');compile_cpp(source,executable)
        run([executable,BUILD/'characters'/args.character/'character.dll',args.asset,args.runtime,output/'poses.bin'])
        for name,yaw in [('front',35),('back',215)]:
            run([sys.executable,FRAMEWORK/'qa/render_equipment.py',output/'poses.bin','--props',args.equipment_directory,'--sheet',output/(name+'.png'),'--yaw',yaw])
        print('Private animation previews: '+str(output)+' (offline visual QA, not live-game verification)')
    elif args.command=='prepare-assets':
        folder,profile=load_profile(args.character)
        recipe=folder/'prepare_assets.py'
        if not recipe.is_file():raise ValueError('This character has no offline asset recipe')
        run([sys.executable,recipe,'--input',args.input,'--output',args.output])
    elif args.command=='build':build(args.character,args.adapter_only,args.with_runtime)
    elif args.command=='doctor':doctor(args.character)
    elif args.command=='test':
        from framework_tests import test
        test(args.character,args.asset,args.runtime)
    elif args.command=='install':
        from framework_install import install
        install(args.character,args.asset,args.runtime,args.check_only,args.collision_directory,args.equipment_directory)


if __name__=='__main__':main()
