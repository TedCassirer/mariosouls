"""Check pinned local dependencies; --setup restores absent clones and our libsm64 changes."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(path, *args):
    return subprocess.run(['git', '-C', str(path), *args], text=True, capture_output=True, check=True).stdout.strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--setup', action='store_true')
    args = parser.parse_args()
    lock = json.loads((ROOT / 'dependencies.lock.json').read_text())
    for dep in lock['build_dependencies']:
        path = ROOT / dep['path']
        if not path.exists():
            if not args.setup:
                raise RuntimeError('Missing dependency: ' + dep['path'] + '. Run --setup.')
            path.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run(['git', 'clone', '--no-checkout', dep['url'], str(path)], check=True)
            git(path, 'checkout', '--detach', dep['commit'])
        if git(path, 'rev-parse', 'HEAD') != dep['commit']:
            raise RuntimeError('Dependency revision changed; preserving checkout: ' + dep['path'])
        expected = dep.get('modified_files', {})
        correct = all((path / name).is_file() and sha(path / name) == value for name, value in expected.items())
        if not correct:
            if not args.setup or git(path, 'status', '--porcelain', '--untracked-files=normal'):
                raise RuntimeError('Dependency changes differ from checkpoint; preserving files: ' + dep['path'])
            git(path, 'apply', '--check', str(ROOT / dep['patch']))
            git(path, 'apply', str(ROOT / dep['patch']))
            for source, target in dep.get('overlay', {}).items():
                shutil.copy2(ROOT / source, path / target)
            if not all(sha(path / name) == value for name, value in expected.items()):
                raise RuntimeError('Dependency patch verification failed')
        tracked = set(git(path, 'diff', '--name-only', 'HEAD').splitlines())
        new = set(git(path, 'ls-files', '--others', '--exclude-standard').splitlines())
        if (tracked | new) - set(expected):
            raise RuntimeError('Additional dependency edits need a new checkpoint: ' + dep['path'])
        print('Verified ' + dep['path'])
    compiler = ROOT / lock['compiler']['path']
    if not compiler.is_file() or sha(compiler) != lock['compiler']['sha256']:
        raise RuntimeError('Restore Zig 0.13.0 at ' + str(compiler) + ' from the local tool archive listed in the lock file')
    print('Verified Zig compiler; dependencies are ready')


if __name__ == '__main__':
    main()
