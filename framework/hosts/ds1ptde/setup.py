"""PTDE install/uninstall implementation for the single MarioSouls.ps1 entry point.

No downloads, game control, save access or executable patching. Builds use the
existing framework entry point. Owned assets stay at their existing local paths.
"""

import argparse
import configparser
import contextlib
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import uuid

sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[3]
EDITION = json.loads(Path(__file__).with_name("edition.json").read_text(encoding="utf-8"))
BUILD = ROOT / "work/framework-build"
STATE = "SoulsCharacterFramework/setup/install.json"
SETUP = "SoulsCharacterFramework/setup"
DSFIX_SHA = "c6855c833763da5872fc2feebfc9e2a87d0c3332b11485d535267bfc14f10e0d"
HOST = "MarioSouls-host.dll"
CHARACTER = "SoulsCharacterFramework/characters/mario64/"
BINARY_PATHS = {"dinput8.dll", HOST, CHARACTER + "character.dll", CHARACTER + "sm64.dll"}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def file_hash(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def child(base, relative):
    """Only literal relative paths, without aliases out of the installation."""
    parts = relative.replace("\\", "/").split("/")
    if not parts or any(p in ("", ".", "..") or ":" in p for p in parts):
        raise ValueError("Unsafe setup path: " + relative)
    path = base.joinpath(*parts)
    for part in [path, *path.parents]:
        if part == base:
            break
        if part.is_symlink() or (
            part.exists() and getattr(part.lstat(), "st_file_attributes", 0) & 0x400
        ):
            raise ValueError("Setup will not write through a link: " + str(part))
    if not path.resolve().is_relative_to(base):
        raise ValueError("Setup path leaves the game directory")
    return path


def read_optional(path):
    if path.exists() and not path.is_file():
        raise ValueError("Expected a file, found a directory: " + str(path))
    return path.read_bytes() if path.is_file() else None


def atomic_write(path, data):
    if data is None:
        path.unlink(missing_ok=True)
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=path.name + ".", suffix=".tmp", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        Path(temporary).unlink(missing_ok=True)


def game_closed(game):
    # Detect the exact installation, not a different private copy. A process
    # that cannot be inspected is conservatively treated as a running game.
    command = (
        "$ErrorActionPreference='Stop'; "
        "Get-Process DARKSOULS -ErrorAction SilentlyContinue | ForEach-Object { "
        "try { if ($_.Path) { $_.Path } else { '<unknown>' } } catch { '<unknown>' } }; exit 0"
    )
    result = subprocess.run(
        ["powershell.exe", "-NoProfile", "-NonInteractive", "-Command", command],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode:
        raise RuntimeError("Could not check running games. No files were changed.")
    executable = game / EDITION["executable"]
    for line in result.stdout.splitlines():
        if line.strip() and (
            line.strip() == "<unknown>" or Path(line.strip()).resolve() == executable
        ):
            raise RuntimeError("Close Dark Souls normally before installing or uninstalling.")


@contextlib.contextmanager
def installation_lock(game):
    # Windows releases the lock even if setup crashes. The harmless lock file
    # remains, avoiding stale-lock recovery commands after a reboot.
    import msvcrt

    path = child(game, SETUP + "/setup.lock")
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a+b") as stream:
        stream.seek(0, os.SEEK_END)
        if stream.tell() == 0:
            stream.write(b"\0")
            stream.flush()
        stream.seek(0)
        try:
            msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
        except OSError as error:
            raise RuntimeError("Another MarioSouls setup is already running.") from error
        try:
            yield
        finally:
            stream.seek(0)
            msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)


def ini_read(data):
    parser = configparser.ConfigParser(interpolation=None)
    parser.read_string(data.decode("utf-8-sig") if data else "")
    normalized = configparser.ConfigParser(interpolation=None)
    for section in parser.sections():
        name = section.lower()
        if normalized.has_section(name):
            raise ValueError("Duplicate configuration section: " + section)
        normalized[name] = dict(parser[section])
    return normalized


def ini_set(data, section, key, value):
    """Patch one key without reformatting the user's remaining tuning/comments."""
    text = data.decode("utf-8-sig") if data else ""
    newline = "\r\n" if "\r\n" in text else "\n"
    lines = text.splitlines(keepends=True)
    start = None
    end = len(lines)
    matches = []
    active = False
    for index, line in enumerate(lines):
        header = re.fullmatch(r"\s*\[([^]]+)\]\s*(?:[;#].*)?", line.strip())
        if header:
            if active:
                end = index
                break
            active = header[1].lower() == section.lower()
            if active:
                start = index
        elif active and re.match(r"\s*" + re.escape(key) + r"\s*=", line, re.I):
            matches.append(index)
    if len(matches) > 1:
        raise ValueError("Duplicate configuration key: " + section + "." + key)
    replacement = key + " = " + str(value) + newline
    if matches:
        lines[matches[0]] = replacement
    elif start is not None:
        if end and not lines[end - 1].endswith(("\n", "\r")):
            lines[end - 1] += newline
        lines.insert(end, replacement)
    else:
        if text and not text.endswith(("\n", "\r")):
            lines.append(newline)
        lines.extend([newline + "[" + section + "]" + newline, replacement])
    return "".join(lines).encode("utf-8")


def wrapper(data, value=None):
    pattern = rb"(?im)^[ \t]*dinput8dllWrapper[ \t]+([^\s#]+)[ \t]*(?:#[^\r\n]*)?(?=\r?$)"
    matches = list(re.finditer(pattern, data))
    if len(matches) != 1:
        raise ValueError("DSfix.ini must contain exactly one dinput8dllWrapper setting.")
    if value is None:
        return matches[0][1].decode("ascii")
    # Replace the value alone, retaining unrelated settings and comments.
    match = matches[0]
    return data[: match.start(1)] + value.encode("ascii") + data[match.end(1) :]


def pe_x86(data, path):
    if len(data) < 64 or data[:2] != b"MZ":
        raise ValueError("Not a PE binary: " + str(path))
    offset = struct.unpack_from("<I", data, 0x3C)[0]
    if offset > len(data) - 26:
        raise ValueError("Invalid PE header: " + str(path))
    if (
        data[offset : offset + 4] != b"PE\0\0"
        or struct.unpack_from("<H", data, offset + 4)[0] != 0x14C
        or struct.unpack_from("<H", data, offset + 24)[0] != 0x10B
    ):
        raise ValueError("PTDE requires x86/PE32 binaries: " + str(path))


def build_files(dry_run):
    receipt = BUILD / "characters/mario64/build.json"
    build = read_json(receipt) if receipt.is_file() else {}
    # Use the same source inventory as the compiler, including newly added files.
    sys.path.insert(0, str(ROOT / "tools"))
    from framework import source_hashes

    if (
        build.get("source_sha256") != source_hashes()
        or not build.get("runtime_sha256")
        or not build.get("host_sha256")
    ):
        if dry_run:
            raise RuntimeError(
                "A current x86 build is needed. A real install will build it automatically; preview does not build or write files."
            )
        print("Building the current PTDE host, Mario adapter and runtime...", flush=True)
        subprocess.run(
            [sys.executable, str(ROOT / "tools/framework.py"), "build", "--with-runtime"],
            cwd=ROOT,
            check=True,
        )
        build = read_json(receipt)
    if (build.get("host"), build.get("character"), build.get("target"), build.get("abi")) != (
        "ds1ptde",
        "mario64",
        "x86-windows-gnu",
        4,
    ):
        raise ValueError("The build manifest is not for PTDE/x86 Mario ABI4.")
    files = {
        "host": BUILD / "dinput8.dll",
        "character": BUILD / "characters/mario64/character.dll",
        "runtime": BUILD / "characters/mario64/sm64.dll",
    }
    for name, path in files.items():
        data = path.read_bytes()
        if digest(data) != build[name + "_sha256" if name != "character" else "adapter_sha256"]:
            raise ValueError("Built file does not match its manifest: " + str(path))
        pe_x86(data, path)
        files[name] = data
    return files, build


def local_assets(args, game, data):
    config = ini_read(data)

    def setting(section, name, fallback=""):
        return config.get(section.lower(), name, fallback=fallback).strip()

    def location(value, description):
        if not value:
            raise ValueError(
                "Missing " + description + ". See docs/INSTALLING.md for first-time setup."
            )
        path = Path(value).expanduser()
        return (game / path).resolve() if not path.is_absolute() else path.resolve()

    rom = location(
        args.rom or setting("Character.mario64", "Asset") or setting("MarioSouls", "ROM"),
        "ROM path (-Rom)",
    )
    with rom.open("rb") as stream:
        header = stream.read(64)
    if (
        rom.stat().st_size != 8 * 1024 * 1024
        or header[:4] != b"\x80\x37\x12\x40"
        or header[0x3E:0x3F] != b"E"
    ):
        raise ValueError("Expected a user-owned US Super Mario 64 8 MiB big-endian .z64 ROM.")
    legacy = setting("MarioSouls", "Collision")
    collision = location(
        args.collision_directory
        or setting("Framework", "CollisionDirectory")
        or (str(Path(legacy).parent) if legacy else ""),
        "collision directory (-CollisionDirectory)",
    )
    equipment = location(
        args.equipment_directory or setting("Framework", "EquipmentDirectory"),
        "equipment directory (-EquipmentDirectory)",
    )
    maps = sorted(collision.glob("m??_??_00_00.bin"))
    props = sorted(equipment.glob("*.scprop"))
    if not maps or not props:
        raise ValueError(
            "Prepared collision maps and equipment props are required; see docs/DEVELOPMENT.md."
        )
    for path in maps:
        with path.open("rb") as stream:
            header = stream.read(8)
        magic, count = struct.unpack("<4sI", header)
        stride = 40 if magic == b"MSC2" else 36
        if (
            magic not in (b"MSC1", b"MSC2")
            or not 0 < count < 2000000
            or path.stat().st_size != 8 + count * stride
        ):
            raise ValueError("Invalid collision cache: " + str(path))
    for path in props:
        with path.open("rb") as stream:
            if stream.read(8) != b"SCPROP1\0":
                raise ValueError("Invalid equipment prop: " + str(path))
    for directory in (collision, equipment):
        manifest_path = directory / "manifest.json"
        if manifest_path.is_file():
            manifest = read_json(manifest_path)
            if manifest.get("game_exe_sha256") != EDITION["supported_exe_sha256"]:
                raise ValueError("Assets were prepared for another executable: " + str(directory))
            for name, expected in manifest.get("files", {}).items():
                if file_hash(child(directory, name)) != expected:
                    raise ValueError("Prepared asset hash mismatch: " + name)
    updates = {
        "Framework": {
            "Character": "mario64",
            "Adapter": game / (CHARACTER + "character.dll"),
            "CollisionDirectory": collision,
            "EquipmentDirectory": equipment,
        },
        "Character.mario64": {"Asset": rom, "Runtime": game / (CHARACTER + "sm64.dll")},
    }
    # Preserve gameplay tuning and the legacy template's directory: it can also
    # contain the optional dynamic Asylum floor used by current gameplay.
    if not legacy or args.collision_directory:
        updates["MarioSouls"] = {"Collision": maps[0]}
    defaults = {
        "Framework": {"StartingClass": 1},
        "MarioSouls": {"HideNative": 1, "EnableCombat": 1, "RenderMode": 1, "DepthTest": 1},
        "World": {"NativeMotion": 1},
    }
    for section, values in defaults.items():
        for key, value in values.items():
            if not config.has_option(section.lower(), key):
                updates.setdefault(section, {})[key] = value
    for section, values in updates.items():
        for key, value in values.items():
            data = ini_set(data, section, key, value)
    return data


def load_state(game):
    path = child(game, STATE)
    if not path.is_file():
        return None
    state = read_json(path)
    if (
        state.get("schema") != 1
        or state.get("host") != "ds1ptde"
        or Path(state["game"]).resolve() != game
    ):
        raise ValueError("Setup record belongs to another installation or version.")
    mode = state.get("loader")
    expected = {CHARACTER + "character.dll", CHARACTER + "sm64.dll", "mariosouls.ini"}
    expected |= {HOST, "DSfix.ini"} if mode == "dsfix" else {"dinput8.dll"}
    if mode not in ("direct", "dsfix") or set(state.get("files", {})) != expected:
        raise ValueError("Invalid setup file ownership record.")
    if mode == "dsfix" and str(state.get("wrapper_before", "")).lower() != "none":
        raise ValueError("Invalid original DSfix wrapper in setup record.")
    for name, record in state["files"].items():
        child(game, name)
        if not re.fullmatch(r"[0-9a-f]{64}", record.get("installed_sha256", "")):
            raise ValueError("Invalid installed hash: " + name)
        original = record.get("original")
        if name in BINARY_PATHS and original is not None:
            raise ValueError("Invalid binary restore ownership: " + name)
        if original:
            relative = original["path"]
            if (
                not relative.startswith(SETUP + "/backups/")
                or file_hash(child(game, relative)) != original["sha256"]
            ):
                raise ValueError("Original backup is missing or modified: " + name)
    return state


def recognized_files(game, state):
    known = (
        {name: record["installed_sha256"] for name, record in state["files"].items()}
        if state
        else {}
    )
    # Adopt only exact DLLs recorded by the former installer. Do not modify its
    # manifests or historical backup archives, or execute its recovery tooling.
    old_path = child(game, "SoulsCharacterFramework/install.json")
    if not state and old_path.is_file():
        old = read_json(old_path)
        if old.get("character") == "mario64":
            known.update(
                {
                    name: value
                    for name, value in old.get("files", {}).items()
                    if name in BINARY_PATHS
                }
            )
    return known


def install_plan(args, game, state):
    config = local_assets(args, game, read_optional(child(game, "mariosouls.ini")))
    known = recognized_files(game, state)
    loader = read_optional(child(game, "dinput8.dll"))
    dsfix = loader is not None and digest(loader) == DSFIX_SHA
    host_name = HOST if dsfix else "dinput8.dll"
    dsfix_data = read_optional(child(game, "DSfix.ini")) if dsfix else None
    before_wrapper = None
    if dsfix:
        if dsfix_data is None:
            raise ValueError("DSfix is installed but DSfix.ini is missing.")
        before_wrapper = wrapper(dsfix_data)
        if before_wrapper.lower() not in ("none", HOST.lower()):
            raise ValueError("DSfix already chains another mod: " + before_wrapper)
    for name in (host_name, CHARACTER + "character.dll", CHARACTER + "sm64.dll"):
        current = read_optional(child(game, name))
        if current is not None and digest(current) != known.get(name):
            raise ValueError("Refusing to overwrite an unrecognized or modified file: " + name)
    if state and state["loader"] != ("dsfix" if dsfix else "direct"):
        raise ValueError(
            "The loader chain changed. Uninstall the recorded setup before changing loaders."
        )
    files, build = build_files(args.dry_run)
    changes = {
        host_name: files["host"],
        CHARACTER + "character.dll": files["character"],
        CHARACTER + "sm64.dll": files["runtime"],
        "mariosouls.ini": config,
    }
    if dsfix:
        changes["DSfix.ini"] = wrapper(dsfix_data, HOST)
    new_state = {
        "schema": 1,
        "host": "ds1ptde",
        "game": str(game),
        "loader": "dsfix" if dsfix else "direct",
        "version": build["framework_version"],
        "commit": build["commit"],
        "files": dict(state["files"]) if state else {},
        "wrapper_before": state["wrapper_before"]
        if state
        else (
            "none" if before_wrapper and before_wrapper.lower() == HOST.lower() else before_wrapper
        ),
    }
    return changes, new_state


def uninstall_plan(game, state):
    if not state:
        print("No installation is managed by this script. Nothing was changed.")
        return {}, None
    changes = {}
    for name, record in state["files"].items():
        current = read_optional(child(game, name))
        if name == "DSfix.ini":
            if current is None or wrapper(current).lower() != HOST.lower():
                raise ValueError("DSfix's wrapper changed; refusing to alter another loader chain.")
            changes[name] = wrapper(current, state["wrapper_before"])
            # Only the wrapper belongs to us. Preserve settings edited between
            # updates as well as those edited after the most recent install.
            continue
        if current is not None and digest(current) != record["installed_sha256"]:
            if name == "mariosouls.ini":
                print("Keeping your edited mariosouls.ini.")
                continue
            raise ValueError("Installed binary was changed; refusing to delete it: " + name)
        original = record["original"]
        changes[name] = child(game, original["path"]).read_bytes() if original else None
    return changes, None


def apply_plan(game, changes, state):
    if not changes:
        return
    # Save every pre-operation byte before the first replacement, including a
    # previous setup record. Retain these small snapshots for manual recovery.
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S") + "-" + uuid.uuid4().hex[:8]
    backup = SETUP + "/backups/" + stamp
    before = {name: read_optional(child(game, name)) for name in [*changes, STATE]}
    for name, data in before.items():
        if data is not None:
            atomic_write(child(game, backup + "/" + name), data)
    for name, data in changes.items():
        if state is not None:
            original = state["files"].get(name, {}).get("original")
            if name not in state["files"] and before[name] is not None and name not in BINARY_PATHS:
                original = {"path": backup + "/" + name, "sha256": digest(before[name])}
            state["files"][name] = {"installed_sha256": digest(data), "original": original}
    changes[STATE] = (json.dumps(state, indent=2) + "\n").encode() if state else None
    touched = []
    try:
        for name, data in changes.items():
            touched.append(name)
            atomic_write(child(game, name), data)
    except BaseException:
        for name in reversed(touched):
            atomic_write(child(game, name), before[name])
        raise
    print("Recovery snapshot: " + str(child(game, backup)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("install", "uninstall"))
    parser.add_argument(
        "--game", default=os.environ.get("MARIOSOULS_GAME_DIR", EDITION["default_game_directory"])
    )
    parser.add_argument("--rom")
    parser.add_argument("--collision-directory")
    parser.add_argument("--equipment-directory")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    if os.name != "nt":
        raise RuntimeError("This setup supports Windows PTDE only.")
    game = Path(args.game).expanduser().resolve()
    if (
        not (game / EDITION["executable"]).is_file()
        and (game / "DATA" / EDITION["executable"]).is_file()
    ):
        game = (game / "DATA").resolve()
    if not game.is_dir():
        raise ValueError("Game directory does not exist: " + str(game))
    if (
        args.action == "install"
        and file_hash(game / EDITION["executable"]) != EDITION["supported_exe_sha256"]
    ):
        raise ValueError("Unsupported Dark Souls executable. Remastered is not supported.")
    if args.action == "uninstall" and not child(game, STATE).is_file():
        print("No installation is managed by this script. Nothing was changed.")
        return
    game_closed(game)
    lock = contextlib.nullcontext() if args.dry_run else installation_lock(game)
    with lock:
        state = load_state(game)
        changes, state = (
            install_plan(args, game, state)
            if args.action == "install"
            else uninstall_plan(game, state)
        )
        print("PTDE directory: " + str(game))
        for name, data in changes.items():
            print(("Remove " if data is None else "Write  ") + name)
        if args.dry_run:
            print("Preview only: no build, backup or game files were written.")
        else:
            game_closed(game)
            if (
                args.action == "install"
                and child(game, STATE).is_file()
                and all(read_optional(child(game, name)) == data for name, data in changes.items())
            ):
                print("MarioSouls is already up to date.")
                return
            apply_plan(game, changes, state)
            print(
                "MarioSouls "
                + args.action
                + " complete. Saves, ROMs and prepared assets are untouched."
            )


if __name__ == "__main__":
    try:
        main()
    except (
        OSError,
        ValueError,
        KeyError,
        RuntimeError,
        subprocess.CalledProcessError,
        struct.error,
        configparser.Error,
    ) as error:
        print("MarioSouls setup: " + str(error), file=sys.stderr)
        sys.exit(1)
