"""Parse and replay native diagnostic fixtures; no installed game or assets."""

import json
from pathlib import Path
import subprocess
import unittest
import tempfile
from unittest.mock import patch
import framework
from html.parser import HTMLParser

EXECUTABLE = framework.BUILD / "diagnostics-test.exe"


class Diagnostics(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads(subprocess.check_output([str(EXECUTABLE), "--fixture"], text=True))

    def test_schema_and_build_identity(self):
        self.assertEqual(self.report["schema"], 1)
        self.assertEqual(self.report["character"], "mario64")
        self.assertEqual(self.report["source_fingerprint"], "fingerprint")
        self.assertFalse(self.report["dirty_build"])

    def test_health_changes(self):
        health = [e for e in self.report["events"] if e["event"] in ("health_loss", "healing")]
        self.assertEqual([(e["before"], e["after"]) for e in health], [(600, 590), (590, 600)])

    def test_transition_and_cache_mismatch(self):
        events = [e["event"] for e in self.report["events"]]
        self.assertIn("map_changed", events)
        self.assertEqual(events.count("collision_map_mismatch"), 1)
        self.assertIn("owner_changed", events)
        self.assertIn("actor_availability", events)

    def test_invalid_and_loading_positions(self):
        self.assertIsNone(self.report["current"]["native_position"])
        self.assertIsNone(self.report["current"]["guest_position"])
        unavailable = next(f for f in self.report["frames"] if not f["actor_available"])
        self.assertIsNone(unavailable["native_position"])
        self.assertIsNone(unavailable["guest_position"])

    def test_no_identity_or_arbitrary_log_fields(self):
        allowed = {
            "time_ms",
            "generation",
            "map",
            "cached_map",
            "owner",
            "hp",
            "max_hp",
            "stamina",
            "max_stamina",
            "action",
            "animation",
            "attack",
            "attack_serial",
            "state_flags",
            "right_equipment",
            "left_equipment",
            "contacts",
            "terrain_triangles",
            "scene_width",
            "scene_height",
            "motion_submissions",
            "motion_acknowledgements",
            "motion_rejections",
            "actor_available",
            "requested",
            "presented",
            "rig_available",
            "focused",
            "controller_connected",
            "menu",
            "world_ready",
            "native_support",
            "combat_enabled",
            "attack_exhausted",
            "native_position",
            "guest_position",
            "guest_floor",
            "guest_speed",
        }
        allowed.remove("menu")
        self.assertEqual(
            set(self.report["current"]),
            allowed | {"damage_pipeline_ready", "native_action_owned", "guest_state_available"},
        )
        self.assertFalse(
            {"name", "username", "path", "save", "rom", "address", "log", "input", "buttons"}
            & set(self.report)
        )

    def test_chronological_history(self):
        times = [f["time_ms"] for f in self.report["frames"]]
        self.assertEqual(times, sorted(times))
        self.assertLessEqual(len(times), 120)

    def test_long_session_is_bounded_and_keeps_newest(self):
        report = json.loads(subprocess.check_output([str(EXECUTABLE), "--stress"], text=True))
        self.assertEqual(len(report["frames"]), 120)
        self.assertEqual(len(report["events"]), 96)
        self.assertEqual(report["frames"][0]["time_ms"], 9880 * 250)
        self.assertEqual(report["frames"][-1]["time_ms"], 9999 * 250)
        self.assertEqual(report["current"], report["frames"][-1])
        self.assertLess(len(json.dumps(report)), 512 * 1024)

    def test_clock_wrap_preserves_insertion_order(self):
        report = json.loads(subprocess.check_output([str(EXECUTABLE), "--wrap"], text=True))
        self.assertEqual([f["time_ms"] for f in report["frames"]], [0xFFFFFF00, 0x100])

    def test_review_form_only_sends_reviewed_title_and_body(self):
        class Review(HTMLParser):
            def __init__(self):
                super().__init__()
                self.forms = []
                self.fields = []
                self.scripts = []

            def handle_starttag(self, tag, attrs):
                attrs = dict(attrs)
                if tag == "form":
                    self.forms.append(attrs)
                if tag in ("input", "textarea"):
                    self.fields.append(attrs.get("name"))
                if tag == "script":
                    self.scripts.append(attrs)

        review = Review()
        review.feed(subprocess.check_output([str(EXECUTABLE), "--review"], text=True))
        self.assertEqual(
            review.forms,
            [{"method": "get", "action": "https://github.com/TedCassirer/mariosouls/issues/new"}],
        )
        self.assertEqual(review.fields, ["title", "body"])
        self.assertFalse(review.scripts)


class BuildIdentity(unittest.TestCase):
    def test_archive_uses_own_metadata_not_parent_repository(self):
        with (
            tempfile.TemporaryDirectory() as tmp,
            patch.object(framework, "ROOT", Path(tmp)),
            patch.object(framework.subprocess, "run") as git,
        ):
            (Path(tmp) / "package-manifest.json").write_text(json.dumps({"commit": "a" * 40}))
            commit, dirty, fingerprint = framework.build_identity({"source": "hash"})
            self.assertEqual(commit, "a" * 40)
            self.assertFalse(dirty)
            self.assertEqual(len(fingerprint), 64)
            git.assert_not_called()

    def test_fingerprint_tracks_content_not_dictionary_order(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(framework, "ROOT", Path(tmp)):
            self.assertEqual(
                framework.build_identity({"a": "1", "b": "2"}),
                framework.build_identity({"b": "2", "a": "1"}),
            )
            self.assertNotEqual(
                framework.build_identity({"a": "1"})[2], framework.build_identity({"a": "2"})[2]
            )

    def test_invalid_archive_identity_is_not_reported(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(framework, "ROOT", Path(tmp)):
            (Path(tmp) / "package-manifest.json").write_text(
                json.dumps({"commit": "C:/Users/private"})
            )
            self.assertEqual(framework.build_identity({})[0], "unknown")


if __name__ == "__main__":
    unittest.main()
