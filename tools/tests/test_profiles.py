"""Manifest boundary checks without a compiler, installed game or owned assets."""

import json
from pathlib import Path
import tempfile
import unittest

from profiles import load_profile, profile_header


class Profiles(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.characters = Path(self.directory.name)
        self.folder = self.characters / "example"
        self.folder.mkdir()
        (self.folder / "adapter.cpp").write_text("// Fixture adapter\n")
        self.profile = {
            "schema": 1,
            "id": "example",
            "source": "adapter.cpp",
            "display_name": "Example",
            "max_triangles": 12,
            "texture": [1, 1],
            "capabilities": [],
            "attacks": [],
            "base_power": 100,
            "upgrade_power": 10,
            "dexterity_weight": 0.5,
        }

    def load(self):
        (self.folder / "character.json").write_text(json.dumps(self.profile))
        return load_profile("example", self.characters)[1]

    def test_defaults_preserve_native_body_and_camera(self):
        profile = self.load()
        self.assertEqual(profile["body"]["model_scale"], 1)
        self.assertEqual(profile["body"]["radius_cm"], 0)
        self.assertEqual(profile["body"]["fall_damage_scale"], 1)
        self.assertFalse(profile["body"]["nonlethal_falls"])
        self.assertEqual(profile["camera"], {"distance_scale": 1.0})

    def test_identity_and_source_cannot_escape_character_directory(self):
        for identifier in ("../example", "Example", "example/other"):
            with self.subTest(identifier=identifier), self.assertRaises(ValueError):
                load_profile(identifier, self.characters)
        for source in ("../adapter.cpp", "C:/adapter.cpp", "adapter.cpp\n#include <evil>"):
            with self.subTest(source=source):
                self.profile["source"] = source
                with self.assertRaises(ValueError):
                    self.load()

    def test_unknown_sdk_capability_is_rejected(self):
        self.profile["capabilities"] = ["write_game_memory"]
        with self.assertRaisesRegex(ValueError, "Unknown capability"):
            self.load()

    def test_body_settings_require_finite_numbers_and_known_fields(self):
        for body in (
            {"radius_cm": float("nan")},
            {"model_scale": True},
            {"model_scale": 3},
            {"nonlethal_falls": 1},
            {"invincible": True},
        ):
            with self.subTest(body=body):
                self.profile["body"] = body
                with self.assertRaises(ValueError):
                    self.load()

    def test_camera_setting_cannot_shrink_or_exceed_sdk_range(self):
        for scale in (0.5, 2.1, True, float("inf")):
            with self.subTest(scale=scale):
                self.profile["camera"] = {"distance_scale": scale}
                with self.assertRaises(ValueError):
                    self.load()

    def test_automatic_attack_must_reference_a_declared_attack(self):
        self.profile["automatic_attack"] = "missing"
        with self.assertRaisesRegex(ValueError, "Automatic attack does not exist"):
            self.load()

    def test_generated_strings_remain_cpp_literals(self):
        self.profile["display_name"] = 'Example"; injected(); //'
        output = self.folder / "profile.generated.h"
        profile_header(self.load(), output)
        self.assertIn(json.dumps(self.profile["display_name"]), output.read_text())
        self.assertEqual(len(output.read_text().splitlines()), 3)


if __name__ == "__main__":
    unittest.main()
