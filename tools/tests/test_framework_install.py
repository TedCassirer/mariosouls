"""Isolated installer transaction checks, with a fake game and no save access."""

import json
import struct
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import framework as f
import framework_install as installer


class Installation(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.game = self.root / "game"
        self.build = self.root / "work/framework-build"
        self.folder = self.root / "framework/characters/fixture"
        for p in [
            self.game / "MarioSouls",
            self.build / "characters/fixture",
            self.folder,
            self.root / "release",
            self.root / "framework/hosts/ds1ptde",
        ]:
            p.mkdir(parents=True)
        self.runtime = self.game / "SoulsCharacterFramework/characters/fixture/runtime.dll"
        self.runtime.parent.mkdir(parents=True)
        self.runtime.write_bytes(b"runtime")
        (self.game / "DARKSOULS.exe").write_bytes(b"fake-executable")
        (self.game / "dinput8.dll").write_bytes(b"old-host")
        (self.game / "mariosouls.ini").write_text(
            "[MarioSouls]\nLibrary=" + str(self.runtime) + "\n[Combat]\nBasePower=105\n"
        )
        (self.game / "MarioSouls/install.json").write_text(
            json.dumps({"proxy_sha256": f.sha(self.game / "dinput8.dll")})
        )
        (self.game / "MarioSouls/harness.ini").write_text("[Harness]\nArmed=1\nHeartbeat=123\n")
        (self.build / "dinput8.dll").write_bytes(b"new-host")
        (self.build / "characters/fixture/character.dll").write_bytes(b"adapter")
        self.profile = {
            "status": "implemented",
            "assets": {"required": False, "runtime": "runtime.dll"},
        }
        (self.folder / "character.json").write_text(json.dumps(self.profile))
        expected = {
            "adapter_sha256": f.sha(self.build / "characters/fixture/character.dll"),
            "host_sha256": f.sha(self.build / "dinput8.dll"),
        }
        (self.build / "characters/fixture/build.json").write_text(
            json.dumps({**expected, "framework_version": "0.9.0", "source_sha256": {}})
        )
        (self.build / "test-report.json").write_text(
            json.dumps(
                {
                    **expected,
                    "status": "passed",
                    "character": "fixture",
                    "runtime_sha256": f.sha(self.runtime),
                }
            )
        )
        (self.root / "framework/hosts/ds1ptde/edition.json").write_text(
            json.dumps(
                {
                    "executable": "DARKSOULS.exe",
                    "supported_exe_sha256": f.sha(self.game / "DARKSOULS.exe"),
                }
            )
        )
        self.patches = [
            patch.object(f, "ROOT", self.root),
            patch.object(f, "GAME", self.game),
            patch.object(f, "BUILD", self.build),
            patch.object(f, "load_profile", return_value=(self.folder, self.profile)),
            patch.object(installer, "require_closed"),
        ]
        for p in self.patches:
            p.start()

    def tearDown(self):
        for p in reversed(self.patches):
            p.stop()
        self.temp.cleanup()

    def call(self, check=False):
        installer.install("fixture", None, str(self.runtime), check)

    def test_check_does_not_mutate(self):
        before = {str(p): f.sha(p) for p in self.root.rglob("*") if p.is_file()}
        self.call(True)
        self.assertEqual(before, {str(p): f.sha(p) for p in self.root.rglob("*") if p.is_file()})

    def test_unknown_loader_requires_explicit_override(self):
        (self.game / "dinput8.dll").write_bytes(b"other-loader")
        with self.assertRaisesRegex(RuntimeError, "loader"):
            self.call(True)
        installer.install("fixture", None, str(self.runtime), replace_loader=True)
        backup = Path(
            json.loads((self.game / "SoulsCharacterFramework/install.json").read_text())["backup"]
        )
        self.assertEqual((backup / "dinput8.dll").read_bytes(), b"other-loader")

    def test_fresh_install_without_legacy_manifest(self):
        (self.game / "dinput8.dll").unlink()
        (self.game / "MarioSouls/install.json").unlink()
        self.call()
        self.assertEqual((self.game / "dinput8.dll").read_bytes(), b"new-host")

    def test_reinstall_runtime_at_destination(self):
        self.call()
        self.assertEqual((self.game / "dinput8.dll").read_bytes(), b"new-host")
        self.assertEqual(self.runtime.read_bytes(), b"runtime")
        self.assertEqual(
            json.loads((self.game / "SoulsCharacterFramework/install.json").read_text())[
                "framework_version"
            ],
            "0.9.0",
        )
        self.assertIn("Armed = 0", (self.game / "MarioSouls/harness.ini").read_text())
        backups = list((self.root / "work/install-backups").glob("before-framework-*"))
        self.assertEqual((backups[0] / "dinput8.dll").read_bytes(), b"old-host")

    def test_process_guard(self):
        with patch.object(installer, "require_closed", side_effect=RuntimeError("Running")):
            with self.assertRaisesRegex(RuntimeError, "Running"):
                self.call()
        self.assertEqual((self.game / "dinput8.dll").read_bytes(), b"old-host")

    def test_untested_binary(self):
        (self.build / "dinput8.dll").write_bytes(b"different")
        with self.assertRaisesRegex(RuntimeError, "exact build"):
            self.call()

    def test_shared_report_cannot_authorize_install(self):
        (self.build / "test-report.json").write_text(
            json.dumps(dict(status="passed", scope="asset-free-shared", authorizes_install=False))
        )
        with self.assertRaises(RuntimeError):
            self.call(True)

    def cache(self):
        cache = self.root / "cache"
        cache.mkdir()
        file = cache / "m18_01_00_00.bin"
        file.write_bytes(b"MSC1" + struct.pack("<I9f", 1, *([0.0] * 9)))
        (cache / "manifest.json").write_text(
            json.dumps(
                dict(
                    schema=1,
                    policy="event-colliders-native",
                    game_exe_sha256=f.sha(self.game / "DARKSOULS.exe"),
                    files={file.name: f.sha(file)},
                )
            )
        )
        return cache

    def test_collision_install_and_preservation(self):
        cache = self.cache()
        installer.install("fixture", None, str(self.runtime), False, str(cache))
        self.assertEqual(
            (cache / "m18_01_00_00.bin").read_bytes(),
            (self.game / "SoulsCharacterFramework/collision/m18_01_00_00.bin").read_bytes(),
        )
        self.call()
        self.assertIn("CollisionDirectory = ", (self.game / "mariosouls.ini").read_text())

    def test_typed_collision_filters(self):
        cache = self.cache()
        file = cache / "m18_01_00_00.bin"
        manifest = cache / "manifest.json"
        for filter_id, coordinate, valid in [
            (15, 0.0, True),
            (13, 0.0, True),
            (256, 0.0, False),
            (15, float("nan"), False),
        ]:
            file.write_bytes(b"MSC2" + struct.pack("<II9f", 1, filter_id, coordinate, *([0.0] * 8)))
            m = json.loads(manifest.read_text())
            m["files"][file.name] = f.sha(file)
            manifest.write_text(json.dumps(m))
            if valid:
                self.assertIn(
                    self.game / "SoulsCharacterFramework/collision" / file.name,
                    installer.collision_files(cache),
                )
            else:
                with self.assertRaisesRegex(RuntimeError, "filter or geometry"):
                    installer.collision_files(cache)

    def test_collision_hash_rejection(self):
        cache = self.cache()
        (cache / "m18_01_00_00.bin").write_bytes(b"wrong")
        with self.assertRaisesRegex(RuntimeError, "hash mismatch"):
            installer.install("fixture", None, str(self.runtime), False, str(cache))
        self.assertEqual((self.game / "dinput8.dll").read_bytes(), b"old-host")

    def equipment(self):
        cache = self.root / "equipment"
        cache.mkdir()
        file = cache / "wp_a_0201.scprop"
        vertices = [(0, 0, 0, 0, 1, 0, 0, 0), (1, 0, 0, 0, 1, 0, 1, 0), (0, 0, 1, 0, 1, 0, 0, 1)]
        bounds = [0.0, 0.0, 0.0, 1.0, 0.0, 1.0]
        file.write_bytes(
            struct.pack("<8s4I6f", b"SCPROP1\0", 3, 1, 1, 1, *bounds)
            + b"".join(struct.pack("<8f", *v) for v in vertices)
            + struct.pack("<3I", 0, 1, 2)
            + bytes([100, 90, 80, 255])
        )
        model = dict(
            file=file.name,
            sha256=f.sha(file),
            vertices=3,
            triangles=1,
            width=1,
            height=1,
            bounds=bounds,
            kind="sword",
            blade_length=1.0,
        )
        manifest = dict(
            schema=1,
            kind="ds1ptde-equipment-props",
            game_exe_sha256=f.sha(self.game / "DARKSOULS.exe"),
            files={file.name: f.sha(file)},
            models={"201": model},
            equipment={"201000": dict(base_id=201000, model=201, kind="sword", name="Test sword")},
        )
        (cache / "manifest.json").write_text(json.dumps(manifest))
        return cache

    def edit_equipment(self, cache, mutate=None, data=None):
        file = cache / "wp_a_0201.scprop"
        manifest = json.loads((cache / "manifest.json").read_text())
        if data is not None:
            file.write_bytes(data)
            manifest["files"][file.name] = f.sha(file)
            manifest["models"]["201"]["sha256"] = f.sha(file)
        if mutate:
            mutate(manifest)
        (cache / "manifest.json").write_text(json.dumps(manifest))

    def test_equipment_install_and_preservation(self):
        cache = self.equipment()
        installer.install("fixture", None, str(self.runtime), equipment=cache)
        destination = self.game / "SoulsCharacterFramework/equipment"
        self.assertEqual(
            (cache / "wp_a_0201.scprop").read_bytes(),
            (destination / "wp_a_0201.scprop").read_bytes(),
        )
        self.assertEqual(
            (cache / "manifest.json").read_bytes(), (destination / "manifest.json").read_bytes()
        )
        receipt = json.loads((self.game / "SoulsCharacterFramework/install.json").read_text())
        self.assertIn("SoulsCharacterFramework/equipment/wp_a_0201.scprop", receipt["files"])
        self.call()
        self.assertIn(
            "EquipmentDirectory = " + str(destination), (self.game / "mariosouls.ini").read_text()
        )

    def test_equipment_check_does_not_mutate(self):
        cache = self.equipment()
        before = {str(p): f.sha(p) for p in self.root.rglob("*") if p.is_file()}
        installer.install("fixture", None, str(self.runtime), check_only=True, equipment=cache)
        self.assertEqual(before, {str(p): f.sha(p) for p in self.root.rglob("*") if p.is_file()})

    def test_equipment_hash_rejection(self):
        cache = self.equipment()
        file = cache / "wp_a_0201.scprop"
        file.write_bytes(file.read_bytes()[:-1] + b"\0")
        with self.assertRaisesRegex(RuntimeError, "hash mismatch"):
            installer.install("fixture", None, str(self.runtime), equipment=cache)
        self.assertEqual((self.game / "dinput8.dll").read_bytes(), b"old-host")

    def test_equipment_rejects_valid_hash_with_truncated_binary(self):
        cache = self.equipment()
        file = cache / "wp_a_0201.scprop"
        self.edit_equipment(cache, data=file.read_bytes()[:-4])
        with self.assertRaisesRegex(RuntimeError, "binary length"):
            installer.equipment_files(cache)

    def test_equipment_rejects_out_of_range_triangle(self):
        cache = self.equipment()
        file = cache / "wp_a_0201.scprop"
        data = bytearray(file.read_bytes())
        struct.pack_into("<I", data, 48 + 3 * 32, 3)
        self.edit_equipment(cache, data=data)
        with self.assertRaisesRegex(RuntimeError, "invalid vertex"):
            installer.equipment_files(cache)

    def test_equipment_rejects_false_bounds(self):
        cache = self.equipment()
        file = cache / "wp_a_0201.scprop"
        data = bytearray(file.read_bytes())
        struct.pack_into("<f", data, 24 + 3 * 4, 2.0)
        self.edit_equipment(
            cache, data=data, mutate=lambda m: m["models"]["201"]["bounds"].__setitem__(3, 2.0)
        )
        with self.assertRaisesRegex(RuntimeError, "declared bounds"):
            installer.equipment_files(cache)

    def test_equipment_rejects_nonfinite_vertex(self):
        cache = self.equipment()
        file = cache / "wp_a_0201.scprop"
        data = bytearray(file.read_bytes())
        struct.pack_into("<f", data, 48, float("nan"))
        self.edit_equipment(cache, data=data)
        with self.assertRaisesRegex(RuntimeError, "Nonfinite"):
            installer.equipment_files(cache)

    def test_equipment_rejects_unsupported_game(self):
        cache = self.equipment()
        (self.game / "DARKSOULS.exe").write_bytes(b"other-version")
        with self.assertRaisesRegex(RuntimeError, "Unsupported Dark Souls"):
            installer.equipment_files(cache)

    def test_equipment_rejects_unknown_model_mapping(self):
        cache = self.equipment()
        self.edit_equipment(
            cache, mutate=lambda m: m["equipment"]["201000"].__setitem__("model", 999)
        )
        with self.assertRaisesRegex(RuntimeError, "mapping mismatch"):
            installer.equipment_files(cache)

    def test_equipment_rejects_unsafe_filename(self):
        cache = self.equipment()
        self.edit_equipment(
            cache, mutate=lambda m: m["models"]["201"].__setitem__("file", "../wp_a_0201.scprop")
        )
        with self.assertRaisesRegex(RuntimeError, "Unsafe equipment filename"):
            installer.equipment_files(cache)

    def test_equipment_rollback_after_partial_copy(self):
        cache = self.equipment()
        destination = self.game / "SoulsCharacterFramework/equipment"
        destination.mkdir()
        (destination / "wp_a_0201.scprop").write_bytes(b"previous prop")
        (destination / "manifest.json").write_bytes(b"previous manifest")
        before = {
            str(p.relative_to(self.game)): f.sha(p) for p in self.game.rglob("*") if p.is_file()
        }
        original = installer.shutil.copy2

        def once(src, dst, *a, **kw):
            if Path(dst) == destination / "manifest.json" and not getattr(once, "failed", False):
                once.failed = True
                raise OSError("equipment copy failure")
            return original(src, dst, *a, **kw)

        with patch.object(installer.shutil, "copy2", side_effect=once):
            with self.assertRaisesRegex(OSError, "equipment copy failure"):
                installer.install("fixture", None, str(self.runtime), equipment=cache)
        self.assertEqual(
            before,
            {str(p.relative_to(self.game)): f.sha(p) for p in self.game.rglob("*") if p.is_file()},
        )

    def test_rollback_after_partial_copy(self):
        original = installer.shutil.copy2

        # Inject one failure after the host/adapter have been copied, then permit rollback.
        def once(src, dst, *a, **kw):
            if Path(dst) == self.runtime and not getattr(once, "failed", False):
                once.failed = True
                raise OSError("simulated disk failure")
            return original(src, dst, *a, **kw)

        with patch.object(installer.shutil, "copy2", side_effect=once):
            with self.assertRaisesRegex(OSError, "simulated"):
                self.call()
        self.assertEqual((self.game / "dinput8.dll").read_bytes(), b"old-host")
        self.assertFalse((self.runtime.parent / "character.dll").exists())
        self.assertEqual(self.runtime.read_bytes(), b"runtime")


if __name__ == "__main__":
    unittest.main()
