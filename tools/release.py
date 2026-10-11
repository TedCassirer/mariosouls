"""Audit and prepare local development archives. Never creates a public release."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import struct
import subprocess
import zipfile
import framework as f

ROOT=f.ROOT
ROOT_FILES={'README.md','CONTRIBUTING.md','AGENTS.md','LICENSE','THIRD_PARTY.md','CHANGELOG.md','SECURITY.md','requirements-ci.txt','dependencies.lock.json'}
PREFIXES=('framework/','tools/','patches/','docs/','licenses/','release/')
BINARIES={'work/framework-build/dinput8.dll','work/framework-build/characters/mario64/character.dll','work/framework-build/characters/mario64/sm64.dll'}
FORBIDDEN={'.z64','.n64','.v64','.rvz','.iso','.sl2','.scmesh','.scprop','.bin','.dcx','.hkx','.flver','.png','.jpg','.mp4','.exe','.dll','.zip'}
SECRETS=[rb'-----BEGIN (?:OPENSSH|RSA|EC|DSA)? ?PRIVATE KEY-----',rb'\bgh[pousr]_[A-Za-z0-9]{30,}\b',rb'\bgithub_pat_[A-Za-z0-9_]{40,}\b',rb'\bAKIA[A-Z0-9]{16}\b',rb'\bsk-(?:proj-|ant-)?[A-Za-z0-9_-]{32,}\b']


def digest(data):return hashlib.sha256(data).hexdigest()


def safe_name(name):
    p=PurePosixPath(name)
    if not name or '\\' in name or ':' in name or p.is_absolute() or '..' in p.parts or str(p)!=name:
        raise RuntimeError('Unsafe package path: '+name)
    return name


def audit_file(name,data,binary=False):
    safe_name(name)
    if Path(name).name.startswith('.env') or Path(name).suffix.lower() in FORBIDDEN and not binary:
        raise RuntimeError('Excluded asset/binary in package: '+name)
    if any(re.search(pattern,data) for pattern in SECRETS):raise RuntimeError('Possible credential in '+name)
    if not binary and b'\0' in data:raise RuntimeError('Unexpected binary content: '+name)
    if binary:
        if name not in BINARIES or data[:2]!=b'MZ' or len(data)<64:raise RuntimeError('Unexpected runtime binary')
        pe=struct.unpack_from('<I',data,60)[0]
        if pe+24>len(data) or data[pe:pe+4]!=b'PE\0\0' or struct.unpack_from('<H',data,pe+4)[0]!=0x14c:raise RuntimeError('Expected x86 PE runtime')


def git(*args):return subprocess.check_output(['git','-C',str(ROOT),*args],text=True).strip()


def source_files():
    files={}
    for name in git('ls-files').splitlines():
        if name not in ROOT_FILES and not name.startswith(PREFIXES):continue
        path=ROOT/name
        if path.is_symlink() or not path.resolve().is_relative_to(ROOT.resolve()):raise RuntimeError('Source path leaves repository')
        data=path.read_bytes();audit_file(name,data);files[name]=data
    for required in ['LICENSE','THIRD_PARTY.md','release/project.json','release/checklist.json']:
        if required not in files:raise RuntimeError('Required release file is not tracked: '+required)
    return files


def release_blockers():
    checklist=json.loads((ROOT/'release/checklist.json').read_text())
    return [item['id']+': '+item['description'] for item in checklist['checks'] if item['status']!='passed']


def candidate_files():
    build=f.BUILD/'characters/mario64'
    receipt=json.loads((build/'build.json').read_text());tests=json.loads((f.BUILD/'test-report.json').read_text())
    if tests.get('status')!='passed' or tests.get('character')!='mario64':raise RuntimeError('Full Mario tests required; shared CI is insufficient')
    if receipt.get('framework_version')!=json.loads((ROOT/'release/project.json').read_text())['version']:raise RuntimeError('Build version differs from release metadata')
    for key,path in [('host_sha256',f.BUILD/'dinput8.dll'),('adapter_sha256',build/'character.dll'),('runtime_sha256',build/'sm64.dll')]:
        if tests.get(key)!=f.sha(path):raise RuntimeError('Tested binary changed: '+key)
        if key!='runtime_sha256' and receipt.get(key)!=tests[key]:raise RuntimeError('Build receipt mismatch')
    for name,expected in receipt['source_sha256'].items():
        safe_name(name)
        if not (ROOT/name).is_file() or f.sha(ROOT/name)!=expected:raise RuntimeError('Source changed after full tests: '+name)
    files={}
    for name in BINARIES:
        data=(ROOT/name).read_bytes();audit_file(name,data,binary=True);files[name]=data
    for name in ['characters/mario64/build.json','test-report.json']:
        files['work/framework-build/'+name]=(f.BUILD/name).read_bytes()
    return files


def write_archive(path,files,metadata):
    path=Path(path)
    if path.exists() or path.with_suffix(path.suffix+'.sha256').exists():raise RuntimeError('Preserving existing package')
    for name,data in files.items():audit_file(name,data,binary=name in BINARIES)
    manifest={**metadata,'files':{name:digest(data) for name,data in sorted(files.items())}}
    payload=dict(files);payload['package-manifest.json']=(json.dumps(manifest,indent=2,sort_keys=True)+'\n').encode()
    path.parent.mkdir(parents=True,exist_ok=True);temp=path.with_suffix(path.suffix+'.tmp')
    if temp.exists():raise RuntimeError('Preserving existing temporary package')
    try:
        with temp.open('xb') as stream:
            with zipfile.ZipFile(stream,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as bundle:
                for name,data in sorted(payload.items()):
                    info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;bundle.writestr(info,data)
        verify_archive(temp)
        temp.rename(path)
    finally:temp.unlink(missing_ok=True)
    path.with_suffix(path.suffix+'.sha256').write_text(digest(path.read_bytes())+'  '+path.name+'\n')
    print('Prepared local '+metadata['kind']+' archive: '+str(path))


def verify_archive(path):
    with zipfile.ZipFile(path) as bundle:
        names=bundle.namelist()
        if len(names)>10000 or sum(i.file_size for i in bundle.infolist())>512*1024**2:raise RuntimeError('Package exceeds size limit')
        if len(set(names))!=len(names) or 'package-manifest.json' not in names:raise RuntimeError('Missing manifest or duplicate package entries')
        for name in names:safe_name(name)
        manifest=json.loads(bundle.read('package-manifest.json'))
        if manifest.get('schema')!=1 or manifest.get('kind') not in ('source','candidate') or manifest.get('public_release') is not False:raise RuntimeError('Invalid development package metadata')
        if manifest['kind']=='source' and BINARIES.intersection(names):raise RuntimeError('Source archive contains runtime binaries')
        if set(names)!=set(manifest['files'])|{'package-manifest.json'}:raise RuntimeError('Package inventory mismatch')
        for name,expected in manifest['files'].items():
            data=bundle.read(name);audit_file(name,data,binary=name in BINARIES)
            if digest(data)!=expected:raise RuntimeError('Package hash mismatch: '+name)
    return manifest


def main():
    parser=argparse.ArgumentParser(description=__doc__);commands=parser.add_subparsers(dest='command',required=True)
    p=commands.add_parser('check');p.add_argument('--release-ready',action='store_true')
    p=commands.add_parser('package');p.add_argument('--kind',choices=['source','candidate'],default='source');p.add_argument('--output',type=Path,required=True)
    p=commands.add_parser('verify');p.add_argument('archive',type=Path)
    args=parser.parse_args()
    if args.command=='verify':
        manifest=verify_archive(args.archive);print('Package inventory and hashes verified: '+manifest['kind']);return
    files=source_files();print('Audited '+str(len(files))+' source files; no game assets, binaries or common credential patterns found.')
    blockers=release_blockers()
    if args.command=='check':
        for item in blockers:print('PENDING '+item)
        if args.release_ready and blockers:raise RuntimeError('Public release gate is not satisfied')
        return
    if git('status','--porcelain','--untracked-files=no'):raise RuntimeError('Commit tracked changes before packaging')
    if args.kind=='candidate':files.update(candidate_files())
    # Minimal compatibility metadata, not the original machine's private archive inventory.
    original=json.loads((ROOT/'checkpoints/mariosouls-v0.7.json').read_text())
    files['checkpoints/mariosouls-v0.7.json']=(json.dumps({'game_exe_sha256':original['game_exe_sha256']},indent=2)+'\n').encode()
    project=json.loads((ROOT/'release/project.json').read_text())
    metadata={'schema':1,'kind':args.kind,'version':project['version'],'commit':git('rev-parse','HEAD'),'public_release':False,'release_blockers':blockers,'requires_user_assets':True}
    write_archive(args.output,files,metadata)


if __name__=='__main__':main()
