"""Restore one installation transaction, retaining unrelated files and saves."""

import datetime
import json
from pathlib import Path, PurePosixPath
import shutil
import tempfile
import framework as f
from framework_install import require_closed


def owned_path(name):
    parts = PurePosixPath(name).parts
    if (
        not name
        or "\\" in name
        or ":" in name
        or name.startswith("/")
        or ".." in parts
        or str(PurePosixPath(name)) != name
    ):
        raise RuntimeError("Unsafe uninstall path")
    if name not in (
        "dinput8.dll",
        "mariosouls.ini",
        "MarioSouls/install.json",
        "MarioSouls/harness.ini",
    ) and not name.startswith("SoulsCharacterFramework/"):
        raise RuntimeError("Uninstall target is not owned by the framework")
    path = f.GAME / name
    if not path.resolve().is_relative_to(f.GAME.resolve()):
        raise RuntimeError("Uninstall target leaves game directory")
    return path


def uninstall(check_only=False):
    manifest = f.GAME / "SoulsCharacterFramework/install.json"
    installed = json.loads(manifest.read_text())
    base = (f.ROOT / "work/install-backups").resolve()
    backup = Path(installed["backup"]).resolve()
    allowed = (base, (f.ROOT / "work/checkpoints").resolve())
    if not any(backup.is_relative_to(folder) and backup != folder for folder in allowed):
        raise RuntimeError("Backup leaves the installation backup directories")
    previous = json.loads((backup / "manifest.json").read_text())
    hashes = json.loads(
        (backup / "hashes.json").read_text()
    )  # Older archives require their historical restore helper.
    expected = dict(installed["files"])
    expected["SoulsCharacterFramework/install.json"] = f.sha(manifest)
    if set(previous) != set(expected) or set(hashes) != set(previous):
        raise RuntimeError("Uninstall manifest differs from backup inventory")
    for name, digest in expected.items():
        path = owned_path(name)
        if not path.is_file() or f.sha(path) != digest:
            raise RuntimeError("Installed file changed; refusing to overwrite: " + name)
        original = backup / name
        if not original.resolve().is_relative_to(backup):
            raise RuntimeError("Backup file leaves its directory")
        if type(previous[name]) is not bool:
            raise RuntimeError("Invalid backup state")
        if previous[name] and (not original.is_file() or f.sha(original) != hashes[name]):
            raise RuntimeError("Backup hash mismatch: " + name)
        if not previous[name] and hashes[name] is not None:
            raise RuntimeError("Invalid absent-file backup hash")
    if check_only:
        print("Uninstall checks passed. No files changed.")
        return
    require_closed()
    recovery = base / ("before-uninstall-" + datetime.datetime.now().strftime("%Y%m%d-%H%M%S-%f"))
    recovery.mkdir(parents=True)
    with tempfile.TemporaryDirectory(dir=recovery) as temp:
        stage = Path(temp)
        for name in expected:
            copy = recovery / name
            copy.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(owned_path(name), copy)
            if previous[name]:
                copy = stage / name
                copy.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(backup / name, copy)
        require_closed()
        written = []
        try:
            for name in expected:
                path = owned_path(name)
                if f.sha(path) != expected[name]:
                    raise RuntimeError("File changed during uninstall: " + name)
                written.append(name)
                if previous[name]:
                    shutil.copy2(stage / name, path)
                    if f.sha(path) != hashes[name]:
                        raise RuntimeError("Post-uninstall hash mismatch")
                else:
                    path.unlink()
        except BaseException:
            for name in reversed(written):
                shutil.copy2(recovery / name, owned_path(name))
            raise
    print(
        "Restored the previous installation. Unrelated files and saves were preserved. Recovery copy: "
        + str(recovery)
    )
