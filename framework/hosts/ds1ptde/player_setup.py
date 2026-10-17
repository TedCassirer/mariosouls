"""Player installer bootstrap. Downloads/builds locally; never downloads a ROM.

The Windows front end supplies a verified source snapshot and Python runtime.
Use the existing PTDE setup backend for all game-directory changes and recovery.
"""

from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(Path(__file__).parent))
import setup


def download(item, cache):
    target = cache / item["filename"]
    if target.exists():
        if setup.file_hash(target) != item["sha256"]:
            raise ValueError("Cached download changed: " + target.name)
        return target
    print("Downloading " + target.name, flush=True)
    cache.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(dir=cache, suffix=".download")
    try:
        with os.fdopen(fd, "wb") as dst:
            with urllib.request.urlopen(item["url"], timeout=60) as src:
                size = 0
                while block := src.read(1024 * 1024):
                    size += len(block)
                    if size > item["size"]:
                        raise ValueError("Download exceeds its pinned size")
                    dst.write(block)
        if size != item["size"] or setup.file_hash(Path(temporary)) != item["sha256"]:
            raise ValueError("Download hash/size mismatch: " + target.name)
        os.replace(temporary, target)
    finally:
        Path(temporary).unlink(missing_ok=True)
    return target


def unpack(archive, destination):
    # A download never supplies absolute paths, links or traversal components.
    with zipfile.ZipFile(archive) as bundle:
        if sum(e.file_size for e in bundle.infolist()) > 2 * 1024**3:
            raise ValueError("Unpacked tool is too large")
        for entry in bundle.infolist():
            if (entry.external_attr >> 16) & 0o170000 == 0o120000:
                raise ValueError("Tool archive contains a symlink")
            setup.child(destination, entry.filename.rstrip("/"))
        bundle.extractall(destination)


def run(*args, cwd=ROOT):
    print("Running " + Path(str(args[0])).name + " " + " ".join(map(str, args[1:3])), flush=True)
    subprocess.run(list(map(str, args)), cwd=cwd, check=True)


def dependencies(lock):
    cache = ROOT.parent / "downloads"
    wheels = [download(item, cache) for item in lock["wheels"]]
    pip = next(p for p in wheels if p.name.startswith("pip-"))
    site = Path(sys.executable).parent / "Lib/site-packages"
    if not (site / "pip").is_dir():
        unpack(pip, site)
    packages = ROOT / "work/python-deps"
    requirements = ROOT.parent / "installer-requirements.txt"
    requirements.write_text(
        "\n".join(
            item["name"] + "==" + item["version"] + " --hash=sha256:" + item["sha256"]
            for item in lock["wheels"]
            if item["name"] != "pip"
        )
        + "\n",
        encoding="utf-8",
    )
    if not (packages / ".installer-ready").exists():
        if packages.exists():
            raise ValueError(
                "Incomplete dependency cache; preserve it and use a fresh installer cache"
            )
        packages.parent.mkdir(parents=True, exist_ok=True)
        staging = Path(tempfile.mkdtemp(prefix="python-deps-", dir=packages.parent))
        run(
            sys.executable,
            "-m",
            "pip",
            "install",
            "--disable-pip-version-check",
            "--no-index",
            "--find-links",
            cache,
            "--require-hashes",
            "--target",
            staging,
            "-r",
            requirements,
        )
        (staging / ".installer-ready").write_text(setup.file_hash(requirements))
        staging.rename(packages)
    elif (packages / ".installer-ready").read_text() != setup.file_hash(requirements):
        raise ValueError("Cached asset dependencies belong to another installer")

    mingit = ROOT.parent / "mingit"
    git_archive = download(lock["git"], cache)
    if not (mingit / "cmd/git.exe").is_file():
        unpack(git_archive, mingit)
    os.environ["PATH"] = str(mingit / "cmd") + os.pathsep + os.environ.get("PATH", "")
    os.environ["GIT_TERMINAL_PROMPT"] = "0"
    os.environ["GIT_CONFIG_GLOBAL"] = os.devnull
    os.environ["GIT_CONFIG_NOSYSTEM"] = "1"
    native = json.loads((ROOT / "dependencies.lock.json").read_text())
    for dep in [*native["build_dependencies"], lock["soulstruct"]]:
        target = ROOT / dep["path"]
        if not target.exists():
            target.parent.mkdir(parents=True, exist_ok=True)
            staging = Path(tempfile.mkdtemp(prefix=target.name + "-", dir=target.parent))
            run("git", "init", "--quiet", "--initial-branch=main", staging)
            run("git", "-C", staging, "config", "core.longpaths", "true")
            # The existing byte-level dependency pins use Windows checkouts.
            # Do not inherit a player's global Git line-ending preferences.
            run("git", "-C", staging, "config", "core.autocrlf", "true")
            run("git", "-C", staging, "remote", "add", "origin", dep["url"])
            run("git", "-C", staging, "fetch", "--depth", "1", "origin", dep["commit"])
            run("git", "-C", staging, "checkout", "--detach", "FETCH_HEAD")
            staging.rename(target)
        actual = subprocess.check_output(
            ["git", "-C", str(target), "rev-parse", "HEAD"], text=True
        ).strip()
        if actual != dep["commit"]:
            raise ValueError("Dependency revision changed: " + dep["path"])
    run(sys.executable, ROOT / "tools/dependencies.py", "--setup", "--download-toolchain")


def assets(game):
    from prepare_collision import prepare as collision
    from prepare_equipment import prepare as equipment

    base = ROOT / "work/player-assets"
    maps = [
        "m10_00",
        "m10_01",
        "m10_02",
        "m11_00",
        "m12_00",
        "m12_01",
        "m13_00",
        "m13_01",
        "m13_02",
        "m14_00",
        "m14_01",
        "m15_00",
        "m15_01",
        "m16_00",
        "m17_00",
        "m18_00",
        "m18_01",
    ]
    combined = base / "collision"
    if not (combined / "manifest.json").exists():
        # Per-map outputs are immutable. Incomplete work remains diagnostic data;
        # the next attempt uses a fresh directory rather than trusting partials.
        staging = Path(tempfile.mkdtemp(prefix="maps-", dir=base))
        files, metadata = {}, {}
        for short in maps:
            map_id = short + "_00_00"
            print("Preparing map " + map_id, flush=True)
            folder = staging / map_id
            collision(game, map_id, folder, native_fallback=True)
            manifest = setup.read_json(folder / "manifest.json")
            target = staging / (map_id + ".bin")
            os.replace(folder / target.name, target)
            files.update(manifest["files"])
            metadata.update(manifest["maps"])
        (staging / "manifest.json").write_text(
            json.dumps(
                {
                    "schema": 1,
                    "policy": "event-colliders-native",
                    "game_exe_sha256": setup.EDITION["supported_exe_sha256"],
                    "files": files,
                    "maps": metadata,
                },
                indent=2,
            ),
            encoding="utf-8",
        )
        staging.rename(combined)
    props = base / "equipment"
    if not props.exists():
        print("Preparing native weapon/shield props", flush=True)
        equipment(game, props)
    return combined, props


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("action", choices=["install", "uninstall", "prepare"])
    p.add_argument("--game", required=True)
    p.add_argument("--rom")
    args = p.parse_args()
    game = Path(args.game).resolve()
    if (game / "DATA/DARKSOULS.exe").is_file():
        game /= "DATA"
    if setup.file_hash(game / "DARKSOULS.exe") != setup.EDITION["supported_exe_sha256"]:
        raise ValueError(
            "Only the supported Windows PTDE executable is accepted; Remastered is not supported"
        )
    setup.game_closed(game)
    if args.action == "uninstall":
        sys.argv = ["setup.py", "uninstall", "--game", str(game)]
        setup.main()
        return
    rom = Path(args.rom or "").resolve()
    with rom.open("rb") as stream:
        header = stream.read(64)
    if rom.stat().st_size != 8 * 1024**2 or header[:4] != b"\x80\x37\x12\x40" or header[62] != 69:
        raise ValueError("Choose your own US Super Mario 64 big-endian .z64 ROM (8 MiB)")
    if not os.access(game, os.W_OK):
        raise PermissionError(
            "No write access to the game directory. Run the installer as administrator if needed."
        )
    lock = setup.read_json(ROOT / "release/installer.lock.json")
    dependencies(lock)
    run(sys.executable, ROOT / "tools/framework.py", "build", "--with-runtime")
    (ROOT / "work/player-assets").mkdir(parents=True, exist_ok=True)
    collision, equipment = assets(game)
    if args.action == "prepare":
        print("Preparation complete. No game files changed.", flush=True)
        return
    sys.argv = [
        "setup.py",
        "install",
        "--game",
        str(game),
        "--rom",
        str(rom),
        "--collision-directory",
        str(collision),
        "--equipment-directory",
        str(equipment),
    ]
    setup.main()


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print("Installation stopped: " + str(error), file=sys.stderr, flush=True)
        sys.exit(1)
