"""Asset-free release, compiler extraction and rollback regression checks."""

import json
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch
import zipfile
import dependencies
import importlib.util
import framework as f
import framework_uninstall as removal
import release
import test_framework_install as install_tests

spec = importlib.util.spec_from_file_location(
    "runtime_geometry", f.FRAMEWORK / "characters/mario64/runtime_geometry.py"
)
geometry = importlib.util.module_from_spec(spec)
spec.loader.exec_module(geometry)


class Archives(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def metadata(self):
        return dict(schema=1, kind="source", public_release=False)

    def test_reproducible_and_verified(self):
        for name in ("one.zip", "two.zip"):
            release.write_archive(self.root / name, {"README.md": b"example"}, self.metadata())
        self.assertEqual((self.root / "one.zip").read_bytes(), (self.root / "two.zip").read_bytes())
        self.assertEqual(
            release.verify_archive(self.root / "one.zip")["files"]["README.md"],
            release.digest(b"example"),
        )

    def test_preserve_existing_archive_and_temp(self):
        output = self.root / "out.zip"
        temp = self.root / "out.zip.tmp"
        temp.write_bytes(b"keep")
        with self.assertRaises(RuntimeError):
            release.write_archive(output, {}, self.metadata())
        self.assertEqual(temp.read_bytes(), b"keep")
        temp.unlink()
        output.write_bytes(b"keep")
        with self.assertRaises(RuntimeError):
            release.write_archive(output, {}, self.metadata())
        self.assertEqual(output.read_bytes(), b"keep")

    def test_reject_assets_credentials_and_traversal(self):
        for name, data in [
            ("assets/mario.z64", b"rom"),
            ("save.sl2", b"save"),
            ("safe.py", b"\x00"),
            (".env", b"test"),
            ("safe.txt", b"gh" + b"p_" + b"a" * 36),
            ("../escape", b"x"),
            ("C:/escape", b"x"),
        ]:
            with self.subTest(name=name), self.assertRaises(RuntimeError):
                release.audit_file(name, data)

    def test_tampering(self):
        path = self.root / "bad.zip"
        manifest = {**self.metadata(), "files": {"README.md": release.digest(b"original")}}
        with zipfile.ZipFile(path, "w") as z:
            z.writestr("README.md", b"changed")
            z.writestr("package-manifest.json", json.dumps(manifest))
        with self.assertRaisesRegex(RuntimeError, "hash mismatch"):
            release.verify_archive(path)

    def test_public_metadata_rejected(self):
        with self.assertRaisesRegex(RuntimeError, "metadata"):
            release.write_archive(
                self.root / "bad.zip", {}, dict(schema=1, kind="source", public_release=True)
            )

    def test_invalid_runtime(self):
        with self.assertRaises(RuntimeError):
            release.audit_file(next(iter(release.BINARIES)), b"not a dll", True)

    def test_toolchain_extraction(self):
        path = self.root / "zig.zip"
        with zipfile.ZipFile(path, "w") as z:
            z.writestr("compiler/zig.exe", b"fixture")
        dependencies.extract_toolchain(path, self.root / "compiler")
        self.assertEqual((self.root / "compiler/zig.exe").read_bytes(), b"fixture")
        with self.assertRaisesRegex(RuntimeError, "Preserving"):
            dependencies.extract_toolchain(path, self.root / "compiler")

    def test_toolchain_traversal(self):
        for entry in (
            "compiler/../escape",
            "other/zig.exe",
            "/compiler/zig.exe",
            "compiler\\zig.exe",
        ):
            path = self.root / "bad.zip"
            with zipfile.ZipFile(path, "w") as z:
                z.writestr(entry, b"fixture")
            # ZipInfo normalizes Windows separators when writing; construct the
            # malformed archive name explicitly to exercise the reader.
            if "\\" in entry:
                path.write_bytes(
                    path.read_bytes().replace(entry.replace("\\", "/").encode(), entry.encode())
                )
            with self.subTest(entry=entry), self.assertRaises(RuntimeError):
                dependencies.extract_toolchain(path, self.root / "compiler")
            self.assertFalse((self.root / "escape").exists())

    def test_compiler_hash_before_extraction(self):
        (self.root / "archive.zip").write_bytes(b"wrong")
        with (
            patch.object(dependencies, "ROOT", self.root),
            self.assertRaisesRegex(RuntimeError, "archive hash"),
        ):
            dependencies.download_toolchain(
                dict(path="compiler/zig.exe", local_archive="archive.zip", archive_sha256="0" * 64)
            )
        self.assertFalse((self.root / "compiler").exists())

    def test_passed_release_gate_requires_evidence(self):
        (self.root / "release").mkdir()
        checklist = dict(
            schema=1,
            checks=[dict(id="live", status="passed", description="Live check", evidence=None)],
        )
        (self.root / "release/checklist.json").write_text(json.dumps(checklist))
        with (
            patch.object(release, "ROOT", self.root),
            self.assertRaisesRegex(RuntimeError, "evidence"),
        ):
            release.release_blockers()

    def test_geometry_rejects_corrupt_cache_before_generation(self):
        (self.root / "work/runtime-geometry-source").mkdir(parents=True)
        (self.root / "work/runtime-geometry-source/geo.inc.c").write_bytes(b"corrupt")
        url = "https://raw.githubusercontent.com/n64decomp/sm64/pinned/actors/mario/geo.inc.c"
        (self.root / "dependencies.lock.json").write_text(
            json.dumps(dict(runtime_geometry=[dict(name="geo.inc.c", url=url, sha256="0" * 64)]))
        )
        with (
            patch.object(
                geometry.runpy, "run_path", return_value=dict(GEO_URL=url, MODEL_URL=url + "model")
            ),
            patch.object(geometry.urllib.request, "urlopen") as network,
            self.assertRaisesRegex(RuntimeError, "hash mismatch"),
        ):
            geometry.prepare_geometry(self.root, self.root / "stage")
        network.assert_not_called()
        self.assertFalse((self.root / "stage").exists())


class Removal(unittest.TestCase):
    setUp = install_tests.Installation.setUp
    tearDown = install_tests.Installation.tearDown
    call = install_tests.Installation.call

    def prepare(self):
        before = {
            p.relative_to(self.game).as_posix(): p.read_bytes()
            for p in self.game.rglob("*")
            if p.is_file()
        }
        self.call()
        return before

    def test_restore_and_preserve_unrelated(self):
        before = self.prepare()
        extra = self.game / "unrelated.txt"
        extra.write_text("keep")
        with patch.object(removal, "require_closed"):
            removal.uninstall()
        after = {
            p.relative_to(self.game).as_posix(): p.read_bytes()
            for p in self.game.rglob("*")
            if p.is_file() and p != extra
        }
        self.assertEqual(before, after)
        self.assertEqual(extra.read_text(), "keep")

    def test_check_only(self):
        self.prepare()
        before = {str(p): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
        removal.uninstall(True)
        self.assertEqual(
            before, {str(p): p.read_bytes() for p in self.root.rglob("*") if p.is_file()}
        )

    def test_legacy_backup_directory(self):
        before = self.prepare()
        path = self.game / "SoulsCharacterFramework/install.json"
        manifest = json.loads(path.read_text())
        original = Path(manifest["backup"])
        legacy = self.root / "work/checkpoints" / original.name
        legacy.parent.mkdir(parents=True)
        shutil.move(str(original), str(legacy))
        manifest["backup"] = str(legacy)
        path.write_text(json.dumps(manifest))
        with patch.object(removal, "require_closed"):
            removal.uninstall()
        self.assertEqual(
            before,
            {
                p.relative_to(self.game).as_posix(): p.read_bytes()
                for p in self.game.rglob("*")
                if p.is_file()
            },
        )

    def test_backup_outside_allowed_directories(self):
        self.prepare()
        path = self.game / "SoulsCharacterFramework/install.json"
        manifest = json.loads(path.read_text())
        manifest["backup"] = str(self.root / "unrelated")
        path.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(RuntimeError, "Backup leaves"):
            removal.uninstall(True)

    def test_user_modified_file(self):
        self.prepare()
        (self.game / "mariosouls.ini").write_text("user changes")
        with self.assertRaisesRegex(RuntimeError, "Installed file changed"):
            removal.uninstall(True)
        self.assertEqual((self.game / "mariosouls.ini").read_text(), "user changes")

    def test_bad_backup(self):
        self.prepare()
        manifest = json.loads((self.game / "SoulsCharacterFramework/install.json").read_text())
        (Path(manifest["backup"]) / "dinput8.dll").write_bytes(b"corrupt")
        with self.assertRaisesRegex(RuntimeError, "Backup hash"):
            removal.uninstall(True)

    def test_running_game(self):
        self.prepare()
        with (
            patch.object(removal, "require_closed", side_effect=RuntimeError("Running")),
            self.assertRaisesRegex(RuntimeError, "Running"),
        ):
            removal.uninstall()
        self.assertEqual((self.game / "dinput8.dll").read_bytes(), b"new-host")

    def test_partial_failure_rolls_back(self):
        self.prepare()
        before = {str(p): p.read_bytes() for p in self.game.rglob("*") if p.is_file()}
        copy = shutil.copy2
        failed = False

        def fail_once(src, dst, *args, **kwargs):
            nonlocal failed
            if Path(dst) == self.game / "mariosouls.ini" and not failed:
                failed = True
                raise OSError("simulated copy failure")
            return copy(src, dst, *args, **kwargs)

        with (
            patch.object(removal, "require_closed"),
            patch.object(removal.shutil, "copy2", side_effect=fail_once),
            self.assertRaisesRegex(OSError, "simulated"),
        ):
            removal.uninstall()
        self.assertTrue(failed)
        self.assertEqual(
            before, {str(p): p.read_bytes() for p in self.game.rglob("*") if p.is_file()}
        )

    def test_unowned_targets(self):
        for name in (
            "../save.sl2",
            "save.sl2",
            "SoulsCharacterFramework/../save.sl2",
            "SoulsCharacterFramework/./config",
        ):
            with self.subTest(name=name), self.assertRaises(RuntimeError):
                removal.owned_path(name)


if __name__ == "__main__":
    unittest.main()
