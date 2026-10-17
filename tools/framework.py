"""Souls Character Framework: build character adapters and prepare local assets."""

from pathlib import Path
import argparse
import datetime
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

from host_editions import edition
from profiles import load_profile as read_profile
from profiles import profile_header as profile_header

ROOT = Path(__file__).resolve().parents[1]
FRAMEWORK = ROOT / "framework"
BUILD = ROOT / "work/framework-build"
ZIG = ROOT / "work/tools/zig-windows-x86_64-0.13.0/zig.exe"
PTDE = edition("ds1ptde")
GAME = Path(os.environ.get("MARIOSOULS_GAME_DIR", PTDE["default_game_directory"]))


def run(args, **kw):
    subprocess.run([str(a) for a in args], check=True, **kw)


def sha(path):
    with path.open("rb") as f:
        return hashlib.file_digest(f, "sha256").hexdigest()


def load_profile(character):
    return read_profile(character, FRAMEWORK / "characters")


def compile_cpp(source, dest, includes=(), extra=(), target="x86-windows-gnu"):
    if target not in ("x86-windows-gnu", "x86_64-windows-gnu"):
        raise ValueError("Unsupported validation target")
    dest.parent.mkdir(parents=True, exist_ok=True)
    args = [
        ZIG,
        "c++",
        "-target",
        target,
        "-std=c++17",
        "-Werror=unused-variable",
        "-Werror=unused-but-set-variable",
        "-O2",
        "-static-libstdc++",
        "-I",
        FRAMEWORK / "sdk",
    ]
    for path in includes:
        args.extend(["-I", path])
    args.extend([source, *extra, "-o", dest])
    log = dest.with_suffix(dest.suffix + ".build.log")
    with log.open("w") as f:
        result = subprocess.run([str(a) for a in args], stdout=f, stderr=subprocess.STDOUT)
    if result.returncode:
        print(log.read_text(encoding="utf-8")[-14000:])
        raise RuntimeError("Build failed: " + str(source))


def build_identity(sources):
    fingerprint = hashlib.sha256(json.dumps(sources, sort_keys=True).encode()).hexdigest()
    commit = "unknown"
    dirty = False
    if (ROOT / ".git").exists():
        try:
            revision = subprocess.run(
                ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True, capture_output=True
            )
            if revision.returncode == 0 and re.fullmatch(
                "[0-9a-f]{40,64}", revision.stdout.strip()
            ):
                commit = revision.stdout.strip()
            status = subprocess.run(
                ["git", "status", "--porcelain", "--untracked-files=normal"],
                cwd=ROOT,
                text=True,
                capture_output=True,
            )
            dirty = status.returncode == 0 and bool(status.stdout.strip())
        except OSError:
            pass
    elif (ROOT / "release/source.json").is_file():
        packaged = json.loads((ROOT / "release/source.json").read_text(encoding="utf-8"))
        if not re.fullmatch("[0-9a-f]{40,64}", packaged.get("commit", "")):
            raise ValueError("Invalid packaged source revision")
        for name, expected in packaged["files"].items():
            path = (ROOT / name).resolve()
            if not path.is_relative_to(ROOT) or not path.is_file() or sha(path) != expected:
                raise ValueError("Packaged source changed: " + name)
        commit = packaged["commit"]
    return commit, dirty, fingerprint


def source_hashes():
    """Source inventory shared by the build manifest and local installer."""
    source_paths = sorted(
        p
        for base in [FRAMEWORK, ROOT / "tools"]
        for p in base.rglob("*")
        if p.is_file() and p.suffix in [".h", ".c", ".cpp", ".def", ".py", ".json", ".in"]
    )
    source_paths.extend(
        [ROOT / "dependencies.lock.json", ROOT / "release/project.json", ROOT / "MarioSouls.ps1"]
    )
    return {path.relative_to(ROOT).as_posix(): sha(path) for path in source_paths}


def build(character, adapter_only=False, with_runtime=False, check_portability=False):
    if check_portability and adapter_only:
        raise ValueError("--check-portability requires a full PTDE build")
    folder, p = load_profile(character)
    out = BUILD / "characters" / character
    out.mkdir(parents=True, exist_ok=True)
    (out / "build.json").unlink(missing_ok=True)
    sources = source_hashes()
    version = json.loads((ROOT / "release/project.json").read_text(encoding="utf-8"))["version"]
    commit, dirty, fingerprint = build_identity(sources)
    (BUILD / "build.generated.h").write_text(
        "\n".join(
            [
                "#pragma once",
                "#define SC_BUILD_VERSION " + json.dumps(version),
                "#define SC_BUILD_COMMIT " + json.dumps(commit),
                "#define SC_BUILD_SOURCE " + json.dumps(fingerprint),
                "#define SC_BUILD_DIRTY " + ("true" if dirty else "false"),
            ]
        )
        + "\n"
    )
    if with_runtime or (check_portability and p["assets"].get("runtime")):
        script = folder / "build_runtime.py"
        if not script.is_file():
            raise ValueError("This adapter has no runtime build script")
        run([sys.executable, script, "--output", out])
    profile_header(p, out / "profile.generated.h")
    compile_cpp(folder / p["source"], out / "character.dll", [out], ["-shared"])
    if not adapter_only:
        objects = []
        for name in ["buffer", "hook", "trampoline", "hde/hde32"]:
            source = ROOT / "work/minhook/src" / f"{name}.c"
            obj = BUILD / (source.stem + ".o")
            run([ZIG, "cc", "-target", "x86-windows-gnu", "-O2", "-c", source, "-o", obj])
            objects.append(obj)
        compile_cpp(
            FRAMEWORK / "hosts/ds1ptde/runtime.cpp",
            BUILD / "dinput8.dll",
            [BUILD],
            extra=[
                "-shared",
                FRAMEWORK / "hosts/ds1ptde/proxy.def",
                *objects,
                "-ld3d9",
                "-ldxguid",
                "-luser32",
                "-lwinmm",
                "-lgdi32",
                "-lshell32",
            ],
        )
    if check_portability:
        from framework_portability import check

        check(character)
    receipt = {
        "framework_version": json.loads(
            (ROOT / "release/project.json").read_text(encoding="utf-8")
        )["version"],
        "abi": 4,
        "host": "ds1ptde",
        "target": PTDE["target"],
        "character": character,
        "profile": p,
        "built_at": datetime.datetime.now().astimezone().isoformat(),
        "adapter_sha256": sha(out / "character.dll"),
        "source_sha256": sources,
        "commit": commit,
        "dirty": dirty,
        "source_fingerprint": fingerprint,
    }
    if (out / "sm64.dll").is_file():
        receipt["runtime_sha256"] = sha(out / "sm64.dll")
    receipt["portability_compiled"] = check_portability
    if not adapter_only:
        receipt["host_sha256"] = sha(BUILD / "dinput8.dll")
    (out / "build.json").write_text(json.dumps(receipt, indent=2))
    print(
        "Built framework host + " + character if not adapter_only else "Built adapter " + character
    )


def scaffold(character, name):
    if not re.fullmatch("[a-z][a-z0-9_]{0,46}", character):
        raise ValueError("Invalid character id")
    dest = FRAMEWORK / "characters" / character
    if dest.exists():
        raise ValueError("Character folder already exists; no files replaced")
    dest.mkdir(parents=True)
    p = {
        "schema": 1,
        "id": character,
        "display_name": name,
        "status": "scaffold",
        "source": "adapter.cpp",
        "max_triangles": 12,
        "texture": [1, 1],
        "audio_rate": 0,
        "capabilities": [],
        "automatic_attack": None,
        "base_power": 100,
        "upgrade_power": 10,
        "dexterity_weight": 0.5,
        "assets": {
            "required": False,
            "description": "Procedural conformance placeholder; replace with an owned-asset adapter",
        },
        "attacks": [],
    }
    (dest / "character.json").write_text(json.dumps(p, indent=2) + "\n")
    shutil.copy2(FRAMEWORK / "templates/adapter.cpp.in", dest / "adapter.cpp")
    (dest / "README.md").write_text(
        "Scaffold only. The procedural triangle is an SDK example, not a game-character port.\nSee framework/docs/ADDING_A_CHARACTER.md.\n"
    )
    print("Created scaffold at " + str(dest))


def asset_recipe(command, character, input_path, output_path):
    """Dispatch an adapter-owned asset recipe without importing its dependencies."""
    folder, _ = load_profile(character)
    filename = {"prepare-assets": "prepare_assets.py", "preview-assets": "preview_assets.py"}[
        command
    ]
    recipe = folder / filename
    if not recipe.is_file():
        raise ValueError("This character has no " + command + " recipe")
    run([sys.executable, recipe, "--input", input_path, "--output", output_path])


def architecture_check():
    errors = []
    for base in ["hosts", "core", "sdk"]:
        for f in (FRAMEWORK / base).rglob("*"):
            if f.suffix not in [".h", ".cpp"]:
                continue
            s = f.read_text(encoding="utf-8")
            if re.search(r"\b(?:SM64\w*|sm64_\w+|libsm64)\b", s):
                errors.append(str(f.relative_to(ROOT)) + ": guest runtime reference")
            if re.search(r"state\.raw(?:Action|Flags)\s*&", s):
                errors.append(str(f.relative_to(ROOT)) + ": opaque guest state used as host logic")
    for base in ["core", "sdk"]:
        for f in (FRAMEWORK / base).rglob("*"):
            if f.suffix not in (".h", ".cpp", ".c"):
                continue
            if re.search(
                r'#include\s*[<"](?:windows|d3d\d*|dinput|.*hosts/).*?[>"]',
                f.read_text(encoding="utf-8"),
                re.I,
            ):
                errors.append(str(f.relative_to(ROOT)) + ": host dependency in portable code")
    for f in (FRAMEWORK / "characters").rglob("*"):
        if f.suffix not in (".h", ".cpp", ".c"):
            continue
        s = f.read_text(encoding="utf-8")
        if re.search(r"\b(?:WriteProcessMemory|ReadProcessMemory|MH_CreateHook)\b", s):
            errors.append(str(f.relative_to(ROOT)) + ": native host access inside adapter")
    if errors:
        raise RuntimeError("\n".join(errors))
    print("Architecture boundaries verified")


def doctor(character):
    architecture_check()
    folder, p = load_profile(character)
    run([sys.executable, ROOT / "tools/dependencies.py"])
    if not ZIG.is_file():
        raise RuntimeError("Zig toolchain missing")
    asset_lock = folder / "asset_dependencies.json"
    if asset_lock.is_file():
        for name, dep in json.loads(asset_lock.read_text(encoding="utf-8")).items():
            if not isinstance(dep, dict):
                continue
            if "exe" in dep:
                tool = ROOT / dep["exe"]
                if not tool.is_file() or sha(tool) != dep["exe_sha256"]:
                    raise RuntimeError("Restore pinned asset tool: " + name)
            if "commit" in dep:
                actual = subprocess.run(
                    ["git", "-C", str(ROOT / dep["path"]), "rev-parse", "HEAD"],
                    capture_output=True,
                    text=True,
                    check=True,
                ).stdout.strip()
                if actual != dep["commit"]:
                    raise RuntimeError("Asset dependency revision differs: " + name)
        print("Pinned asset tools verified")
    print(
        "Profile valid: "
        + p["id"]
        + " ("
        + p["status"]
        + "); "
        + str(len(p["attacks"]))
        + " attacks"
    )


def format_sources(check=False):
    """Apply the pinned project styles, or check them without modifying source."""
    # The repository uses a Git allowlist for private workspaces. Style coverage
    # comes from these two explicit source roots, independently of ignore rules.
    if check:
        run(
            [
                sys.executable,
                "-m",
                "ruff",
                "check",
                "--no-respect-gitignore",
                FRAMEWORK,
                ROOT / "tools",
            ]
        )
    run(
        [
            sys.executable,
            "-m",
            "ruff",
            "format",
            "--no-respect-gitignore",
            *(["--check"] if check else []),
            FRAMEWORK,
            ROOT / "tools",
        ]
    )
    formatter = Path(sys.executable).with_name("clang-format.exe")
    if not formatter.is_file():
        formatter = shutil.which("clang-format")
    if not formatter:
        raise RuntimeError("Install requirements-dev.txt to use the source formatter")
    sources = sorted(path for path in FRAMEWORK.rglob("*") if path.suffix in (".h", ".cpp", ".c"))
    run([formatter, *(["--dry-run", "--Werror"] if check else ["-i"]), *sources])
    template = FRAMEWORK / "templates/adapter.cpp.in"
    run(
        [
            formatter,
            "--assume-filename=adapter.cpp",
            *(["--dry-run", "--Werror"] if check else ["-i"]),
            template,
        ]
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    p = commands.add_parser("format", help="Format active source or check its style")
    p.add_argument("--check", action="store_true")
    for name in ["build", "doctor"]:
        p = commands.add_parser(name)
        p.add_argument("--host", default="ds1ptde")
        p.add_argument("--character", default="mario64")
        if name == "build":
            p.add_argument("--adapter-only", action="store_true")
            p.add_argument("--with-runtime", action="store_true")
            p.add_argument(
                "--check-portability",
                action="store_true",
                help="Also compile x86/x64 SDK/shared headers and x64 guest binaries; executes no tests",
            )
    p = commands.add_parser("new-character")
    p.add_argument("id")
    p.add_argument("--name", required=True)
    for name in ["prepare-assets", "preview-assets"]:
        p = commands.add_parser(name)
        p.add_argument("--character", required=True)
        p.add_argument("--input", required=True)
        p.add_argument("--output", required=True)
    p = commands.add_parser("prepare-collision")
    p.add_argument("--host", default="ds1ptde")
    p.add_argument("--map", required=True)
    p.add_argument("--output", required=True)
    p = commands.add_parser("prepare-equipment")
    p.add_argument("--host", default="ds1ptde")
    p.add_argument("--game", default=str(GAME))
    p.add_argument("--output", required=True)
    p = commands.add_parser("preview-equipment")
    p.add_argument("--actions", choices=["melee", "casting"], default="melee")
    p.add_argument("--character", default="mario64")
    p.add_argument("--asset", required=True)
    p.add_argument("--runtime", required=True)
    p.add_argument("--equipment-directory", required=True)
    p.add_argument("--output", required=True)
    args = parser.parse_args()
    if hasattr(args, "host"):
        edition(args.host)
    if args.command == "format":
        format_sources(args.check)
    elif args.command == "new-character":
        scaffold(args.id, args.name)
    elif args.command == "prepare-collision":
        run(
            [
                sys.executable,
                FRAMEWORK / "hosts/ds1ptde/prepare_collision.py",
                "--game",
                GAME,
                "--map",
                args.map,
                "--output",
                args.output,
            ]
        )
    elif args.command == "prepare-equipment":
        run(
            [
                sys.executable,
                FRAMEWORK / "hosts/ds1ptde/prepare_equipment.py",
                "--game",
                args.game,
                "--output",
                args.output,
            ]
        )
    elif args.command == "preview-equipment":
        folder, profile = load_profile(args.character)
        source = folder / (
            "casting_preview.cpp" if args.actions == "casting" else "equipment_preview.cpp"
        )
        if not source.is_file():
            raise ValueError("This adapter has no equipment preview fixture")
        output = Path(args.output).resolve()
        output.mkdir(parents=True, exist_ok=False)
        executable = BUILD / (args.character + "-equipment-preview.exe")
        compile_cpp(source, executable)
        run(
            [
                executable,
                BUILD / "characters" / args.character / "character.dll",
                args.asset,
                args.runtime,
                output / "poses.bin",
            ]
        )
        for name, yaw in [("front", 35), ("back", 215)]:
            run(
                [
                    sys.executable,
                    FRAMEWORK / "hosts/ds1ptde/preview_equipment.py",
                    output / "poses.bin",
                    "--props",
                    args.equipment_directory,
                    "--sheet",
                    output / (name + ".png"),
                    "--yaw",
                    yaw,
                ]
            )
        print(
            "Private animation previews: "
            + str(output)
            + " (offline visual QA, not live-game verification)"
        )
    elif args.command in ("prepare-assets", "preview-assets"):
        asset_recipe(args.command, args.character, args.input, args.output)
    elif args.command == "build":
        build(args.character, args.adapter_only, args.with_runtime, args.check_portability)
    elif args.command == "doctor":
        doctor(args.character)


if __name__ == "__main__":
    main()
