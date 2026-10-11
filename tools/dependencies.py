"""Check pinned local dependencies; --setup restores absent clones and our libsm64 changes."""

from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(path, *args):
    return subprocess.run(
        ["git", "-C", str(path), *args], text=True, capture_output=True, check=True
    ).stdout.strip()


def extract_toolchain(archive, destination):
    """Extract only a verified archive, refusing traversal and unexpected roots."""
    destination = Path(destination)
    if destination.exists():
        raise RuntimeError("Preserving existing compiler directory")
    with zipfile.ZipFile(archive) as bundle:
        entries = bundle.infolist()
        if not entries or len(entries) > 100000 or sum(e.file_size for e in entries) > 4 * 1024**3:
            raise RuntimeError("Unexpected toolchain archive size")
        for entry in entries:
            parts = entry.filename.split("/")
            if (
                "\\" in entry.orig_filename
                or ":" in entry.filename
                or ".." in parts
                or parts[0] != destination.name
                or (entry.external_attr >> 16) & 0o170000 == 0o120000
            ):
                raise RuntimeError("Unsafe toolchain archive path")
        with tempfile.TemporaryDirectory(dir=destination.parent) as stage:
            bundle.extractall(stage)
            prepared = Path(stage) / destination.name
            if not (prepared / "zig.exe").is_file():
                raise RuntimeError("Compiler missing from archive")
            prepared.rename(destination)


def download_toolchain(compiler):
    exe = ROOT / compiler["path"]
    if exe.is_file():
        return
    archive = ROOT / compiler["local_archive"]
    archive.parent.mkdir(parents=True, exist_ok=True)
    if not archive.is_file():
        url = compiler["url"]
        if not url.startswith("https://ziglang.org/download/"):
            raise RuntimeError("Unexpected compiler download host")
        temp = archive.with_suffix(".download")
        if temp.exists():
            raise RuntimeError("Preserving existing partial compiler download")
        try:
            with urllib.request.urlopen(url, timeout=60) as src, temp.open("xb") as dst:
                shutil.copyfileobj(src, dst)
            if sha(temp) != compiler["archive_sha256"]:
                raise RuntimeError("Compiler archive hash mismatch")
            temp.rename(archive)
        finally:
            temp.unlink(missing_ok=True)
    if sha(archive) != compiler["archive_sha256"]:
        raise RuntimeError("Compiler archive hash mismatch")
    extract_toolchain(archive, exe.parent)
    if sha(exe) != compiler["sha256"]:
        raise RuntimeError("Compiler executable hash mismatch")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--setup", action="store_true")
    parser.add_argument("--download-toolchain", action="store_true")
    args = parser.parse_args()
    lock = json.loads((ROOT / "dependencies.lock.json").read_text())
    if args.download_toolchain:
        download_toolchain(lock["compiler"])
    for dep in lock["build_dependencies"]:
        path = ROOT / dep["path"]
        if not path.exists():
            if not args.setup:
                raise RuntimeError("Missing dependency: " + dep["path"] + ". Run --setup.")
            path.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run(["git", "clone", "--no-checkout", dep["url"], str(path)], check=True)
            git(path, "checkout", "--detach", dep["commit"])
        if git(path, "rev-parse", "HEAD") != dep["commit"]:
            raise RuntimeError("Dependency revision changed; preserving checkout: " + dep["path"])
        expected = dep.get("modified_files", {})
        correct = all(
            (path / name).is_file() and sha(path / name) == value
            for name, value in expected.items()
        )
        if not correct:
            if not args.setup or git(path, "status", "--porcelain", "--untracked-files=normal"):
                raise RuntimeError(
                    "Dependency changes differ from the lock file; preserving files: " + dep["path"]
                )
            git(path, "apply", "--check", str(ROOT / dep["patch"]))
            git(path, "apply", str(ROOT / dep["patch"]))
            for source, target in dep.get("overlay", {}).items():
                shutil.copy2(ROOT / source, path / target)
            if not all(sha(path / name) == value for name, value in expected.items()):
                raise RuntimeError("Dependency patch verification failed")
        tracked = set(git(path, "diff", "--name-only", "HEAD").splitlines())
        new = set(git(path, "ls-files", "--others", "--exclude-standard").splitlines())
        if (tracked | new) - set(expected):
            raise RuntimeError(
                "Additional dependency edits need a lock file update: " + dep["path"]
            )
        print("Verified " + dep["path"])
    compiler = ROOT / lock["compiler"]["path"]
    if not compiler.is_file() or sha(compiler) != lock["compiler"]["sha256"]:
        raise RuntimeError(
            "Restore Zig 0.13.0 at "
            + str(compiler)
            + " from the local tool archive listed in the lock file"
        )
    print("Verified Zig compiler; dependencies are ready")


if __name__ == "__main__":
    main()
