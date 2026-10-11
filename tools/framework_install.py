"""Transactional local install. No ROM copying, save access, or game launching."""
import configparser
import datetime
import json
import re
import struct
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile
import framework as f

def require_closed():
    alive=subprocess.run(['powershell','-NoProfile','-Command','[bool](Get-Process DARKSOULS -ErrorAction SilentlyContinue)'],capture_output=True,text=True,check=True).stdout.strip()
    if alive!='False':raise RuntimeError('Close Dark Souls normally before installing')

def collision_files(directory):
    directory=Path(directory)
    manifest=json.loads((directory/'manifest.json').read_text())
    if manifest.get('schema')!=1 or manifest.get('policy')!='event-colliders-native' or manifest.get('game_exe_sha256')!=f.sha(f.GAME/'DARKSOULS.exe'):raise RuntimeError('Invalid native collision cache manifest')
    sources={}
    if not manifest.get('files'):raise RuntimeError('Empty collision cache')
    for name,digest in manifest['files'].items():
        if not re.fullmatch(r'm\d{2}_\d{2}_\d{2}_\d{2}\.bin',name):raise RuntimeError('Unsafe collision cache filename')
        source=directory/name
        if not source.resolve().is_relative_to(directory.resolve()) or f.sha(source)!=digest:raise RuntimeError('Collision cache hash mismatch')
        data=source.read_bytes()
        if len(data)<8 or data[:4]!=b'MSC1' or not 0<struct.unpack_from('<I',data,4)[0]<2000000 or len(data)!=8+36*struct.unpack_from('<I',data,4)[0]:raise RuntimeError('Invalid collision cache format')
        sources[f.GAME/'SoulsCharacterFramework/collision'/name]=source
    return sources

def install(character,asset,runtime,check_only=False,collision=None):
    folder,p=f.load_profile(character)
    if p['status']!='implemented':raise RuntimeError('Scaffolds cannot be installed as completed characters')
    build=f.BUILD/'characters'/character;receipt=json.loads((build/'build.json').read_text())
    checks=json.loads((f.BUILD/'test-report.json').read_text())
    expected={'adapter_sha256':f.sha(build/'character.dll'),'host_sha256':f.sha(f.BUILD/'dinput8.dll')}
    if checks.get('status')!='passed' or checks.get('character')!=character or any(checks.get(k)!=v for k,v in expected.items()):raise RuntimeError('Run framework tests on this exact build before installing')
    if any(receipt.get(k)!=v for k,v in expected.items()):raise RuntimeError('Build receipt does not match binaries')
    changed=[name for name,h in receipt['source_sha256'].items() if not (f.ROOT/name).is_file() or f.sha(f.ROOT/name)!=h]
    if changed:raise RuntimeError('Sources changed after build; rebuild and retest: '+', '.join(changed))
    checkpoint=json.loads((f.ROOT/'checkpoints/mariosouls-v0.7.json').read_text())
    if f.sha(f.GAME/'DARKSOULS.exe')!=checkpoint['game_exe_sha256']:raise RuntimeError('Unsupported Dark Souls executable')
    cfg=configparser.ConfigParser();cfg.optionxform=str;cfg.read(f.GAME/'mariosouls.ini')
    section='Character.'+character
    asset=asset or cfg.get(section,'Asset',fallback=cfg.get('MarioSouls','ROM',fallback=''))
    runtime=runtime or cfg.get(section,'Runtime',fallback=cfg.get('MarioSouls','Library',fallback=''))
    if p['assets'].get('required') and (not asset or not Path(asset).is_file()):raise RuntimeError('Owned character assets are missing')
    if p['assets'].get('runtime'):
        if not runtime or not Path(runtime).is_file():raise RuntimeError('Character runtime DLL is missing')
        if checks.get('runtime_sha256')!=f.sha(Path(runtime)):raise RuntimeError('Runtime differs from the one used in the adapter tests')
    if p['assets'].get('required') and checks.get('asset_sha256')!=f.sha(Path(asset)):raise RuntimeError('Asset differs from the one used in the adapter tests')
    destination=f.GAME/'SoulsCharacterFramework/characters'/character
    old_character=cfg.get('Framework','Character',fallback='mario64')
    previous_collision=cfg.get('Framework','CollisionDirectory',fallback='')
    prepared_collision=collision_files(collision) if collision else {}
    cfg['Framework']={'Character':character,'Adapter':str(destination/'character.dll'),'ABI':'3'}
    if prepared_collision:cfg['Framework']['CollisionDirectory']=str(f.GAME/'SoulsCharacterFramework/collision')
    elif previous_collision:cfg['Framework']['CollisionDirectory']=previous_collision
    cfg[section]={'Asset':str(Path(asset).resolve()) if asset else '', 'Runtime':str(destination/p['assets']['runtime']) if p['assets'].get('runtime') else ''}
    combat='Combat.'+character
    if combat not in cfg:cfg[combat]=dict(cfg['Combat']) if old_character==character and 'Combat' in cfg else {}
    if 'Feedback' not in cfg:cfg['Feedback']={}
    for key,value in dict(Enabled='1',HitStop='1',Sound='1',Flash='1',Rumble='1',Volume='65',RumbleStrength='75').items():
        if key not in cfg['Feedback']:cfg['Feedback'][key]=value
    for section_name,values in [('Test',{'Mode':'0','Frames':'0','Buttons':'0','X':'0','Y':'0'}),('Harness',{'Armed':'0','Session':'0'})]:
        if section_name not in cfg:cfg[section_name]={}
        cfg[section_name].update(values)
    if check_only:
        print('Install checks passed; exact tested build, runtime, assets and host identity match. No files changed.');return
    require_closed()
    backup=f.ROOT/'work/checkpoints'/('before-framework-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f'));backup.mkdir(parents=True)
    with tempfile.TemporaryDirectory(dir=backup,prefix='stage-') as tmp:
        temp=Path(tmp);sources={f.GAME/'dinput8.dll':f.BUILD/'dinput8.dll',destination/'character.dll':build/'character.dll',destination/'character.json':folder/'character.json'}
        sources.update(prepared_collision)
        if p['assets'].get('runtime'):sources[destination/p['assets']['runtime']]=Path(runtime)
        with (temp/'mariosouls.ini').open('w') as out:cfg.write(out)
        sources[f.GAME/'mariosouls.ini']=temp/'mariosouls.ini'
        harness_path=f.GAME/'MarioSouls/harness.ini'
        if harness_path.is_file():
            harness=configparser.ConfigParser();harness.optionxform=str;harness.read(harness_path)
            if 'Harness' not in harness:harness['Harness']={}
            harness['Harness'].update(Armed='0',Heartbeat='0',Mode='0',Frames='0',Buttons='0')
            with (temp/'harness.ini').open('w') as out:harness.write(out)
            sources[harness_path]=temp/'harness.ini'
        legacy=json.loads((f.GAME/'MarioSouls/install.json').read_text());legacy['version']='framework-0.4.1';legacy['proxy_sha256']=expected['host_sha256'];legacy['framework']={'character':character,**expected}
        (temp/'legacy.json').write_text(json.dumps(legacy,indent=2));sources[f.GAME/'MarioSouls/install.json']=temp/'legacy.json'
        target_manifest=f.GAME/'SoulsCharacterFramework/install.json'
        installed={'framework_version':'0.4.1','character':character,'abi':3,'build':receipt,'backup':str(backup),'files':{str(dst.relative_to(f.GAME)).replace('\\','/'):f.sha(src) for dst,src in sources.items()}}
        (temp/'install.json').write_text(json.dumps(installed,indent=2));sources[target_manifest]=temp/'install.json'
        # Stage every source before touching destinations, including on reinstall
        # when the configured runtime already lives at its destination.
        for index,(dst,src) in enumerate(list(sources.items())):
            staged=temp/('payload-'+str(index));shutil.copy2(src,staged);sources[dst]=staged
        previous={}
        for dst in sources:
            resolved=dst.resolve()
            if not resolved.is_relative_to(f.GAME.resolve()):raise RuntimeError('Install target leaves the game directory')
            name=dst.relative_to(f.GAME).as_posix();previous[name]=dst.exists()
            if dst.exists():
                copy=backup/name;copy.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(dst,copy)
        (backup/'manifest.json').write_text(json.dumps(previous,indent=2))
        written=[]
        require_closed()
        try:
            for dst,src in sources.items():
                dst.parent.mkdir(parents=True,exist_ok=True);written.append(dst);shutil.copy2(src,dst)
                if f.sha(dst)!=f.sha(src):raise RuntimeError('Post-install hash mismatch')
        except BaseException:
            for dst in reversed(written):
                name=dst.relative_to(f.GAME).as_posix()
                if previous[name]:shutil.copy2(backup/name,dst)
                elif dst.exists():dst.unlink()
            raise
    print('Installed framework 0.4.1 with '+character+'; previous files retained at '+str(backup))
