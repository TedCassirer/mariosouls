"""Verify or restore an exact local MarioSouls runtime snapshot. Never touches saves."""
from pathlib import Path, PurePosixPath
import argparse
import datetime
import hashlib
import json
import os
import shutil
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'checkpoints' / 'mariosouls-v0.7.json'


def digest(path):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def safe_path(base, relative):
    p = PurePosixPath(relative)
    if p.is_absolute() or '..' in p.parts or '\\' in relative or ':' in relative:
        raise ValueError('Unsafe snapshot path: ' + relative)
    target = (base / relative).resolve()
    if not target.is_relative_to(base.resolve()) or target == base.resolve():
        raise ValueError('Snapshot path leaves its destination: ' + relative)
    return target


def verify(manifest):
    archive = safe_path(ROOT, manifest['archive'])
    if digest(archive) != manifest['archive_sha256']:
        raise ValueError('Runtime backup hash does not match the committed manifest')
    with zipfile.ZipFile(archive) as z:
        names = z.namelist()
        if len(names) != len(set(names)) or set(names) != set(manifest['files']):
            raise ValueError('Runtime backup file list does not match')
        for name, expected in manifest['files'].items():
            safe_path(ROOT, name)
            content = z.read(name)
            if len(content) != expected['bytes'] or hashlib.sha256(content).hexdigest() != expected['sha256']:
                raise ValueError('Runtime backup file failed verification: ' + name)
    return archive


def game_running():
    result = subprocess.run(
        ['powershell', '-NoProfile', '-NonInteractive', '-Command',
         '[bool](Get-Process DARKSOULS -ErrorAction SilentlyContinue)'],
        text=True, capture_output=True, check=True)
    return result.stdout.strip() != 'False'


def restore(manifest, archive):
    if os.name != 'nt':
        raise RuntimeError('This installed-game restore is Windows only')
    if game_running():
        raise RuntimeError('Quit Dark Souls normally and close it before restoring this checkpoint')
    game = Path(manifest['game_directory']).resolve()
    if digest(game / 'DARKSOULS.exe') != manifest['game_exe_sha256']:
        raise RuntimeError('Installed Dark Souls executable does not match this checkpoint')
    # Resolve every target before any writes; rejects symlink/junction escapes.
    targets = {name: safe_path(game, name) for name in manifest['files']}
    saved = ROOT / 'work' / 'checkpoints' / ('before-restore-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
    saved.mkdir(parents=True)
    previous = {name: path.exists() for name, path in targets.items()}
    for name, path in targets.items():
        if previous[name]:
            dest = safe_path(saved, name)
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, dest)
    (saved / 'files.json').write_text(json.dumps(previous, indent=2))
    written = []
    try:
        with tempfile.TemporaryDirectory(dir=saved, prefix='staging-') as tmp:
            with zipfile.ZipFile(archive) as z:
                for name in targets:
                    staged = safe_path(Path(tmp), name)
                    staged.parent.mkdir(parents=True, exist_ok=True)
                    staged.write_bytes(z.read(name))
            if game_running():
                raise RuntimeError('Dark Souls started during preparation; no runtime files replaced')
            for name, path in targets.items():
                path.parent.mkdir(parents=True, exist_ok=True)
                written.append(name)
                shutil.copy2(safe_path(Path(tmp), name), path)
                if digest(path) != manifest['files'][name]['sha256']:
                    raise RuntimeError('Restored file failed verification: ' + name)
    except BaseException:
        for name in reversed(written):
            if previous[name]:
                shutil.copy2(safe_path(saved, name), targets[name])
            elif targets[name].exists():
                targets[name].unlink()
        raise
    print('Restored MarioSouls 0.7. Character saves were not changed.')
    print('Previous runtime files retained at: ' + str(saved))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--restore', action='store_true', help='Restore runtime after verification; requires the game closed')
    p.add_argument('--installed', action='store_true', help='Also compare current installed runtime, without changes')
    args = p.parse_args()
    manifest = json.loads(MANIFEST.read_text())
    archive = verify(manifest)
    print('Checkpoint archive verified: ' + str(len(manifest['files'])) + ' files')
    if args.installed:
        game = Path(manifest['game_directory'])
        mismatch = [name for name, item in manifest['files'].items()
                    if not safe_path(game, name).is_file() or digest(safe_path(game, name)) != item['sha256']]
        if mismatch:
            raise RuntimeError('Installed files differ from checkpoint: ' + ', '.join(mismatch))
        print('Installed runtime matches the checkpoint exactly')
    if args.restore:
        restore(manifest, archive)


if __name__ == '__main__':
    main()
