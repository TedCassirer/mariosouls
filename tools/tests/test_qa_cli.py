"""QA discovery and argument validation must work without opening native tooling."""

from contextlib import redirect_stderr, redirect_stdout
import builtins
import io
from pathlib import Path
import runpy
import subprocess
import sys
import unittest
from unittest.mock import patch

QA = Path(__file__).resolve().parents[2] / "framework/qa"


class QaCommands(unittest.TestCase):
    def setUp(self):
        self.registry = runpy.run_path(str(QA / "__main__.py"))

    def test_listing_scenarios_does_not_launch_a_process(self):
        with patch("subprocess.run") as launch, redirect_stdout(io.StringIO()) as output:
            self.assertEqual(self.registry["main"]([]), 0)
        launch.assert_not_called()
        self.assertIn("body-probe", output.getvalue())

    def test_unknown_scenario_cannot_be_used_as_a_script_path(self):
        with patch("subprocess.run") as launch, redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as error:
                self.registry["main"](["../../untrusted.py"])
        self.assertEqual(error.exception.code, 2)
        launch.assert_not_called()

    def test_scenario_arguments_are_forwarded_without_a_shell(self):
        arguments = ["critical", "123", "parry", "--output", "a directory with spaces"]
        with patch("subprocess.run", return_value=subprocess.CompletedProcess([], 7)) as launch:
            self.assertEqual(self.registry["main"](arguments), 7)
        launch.assert_called_once_with(
            [sys.executable, str(QA / "critical_smoke.py"), *arguments[1:]]
        )

    def test_importing_any_scenario_does_not_load_game_access_or_control_libraries(self):
        ordinary_import = builtins.__import__

        def guarded_import(name, *args, **kwargs):
            if name.split(".")[0] in {"host", "bridge", "transport", "um", "pefile"}:
                self.fail("Scenario import attempted native tooling: " + name)
            return ordinary_import(name, *args, **kwargs)

        with patch("builtins.__import__", side_effect=guarded_import):
            for filename in self.registry["SCENARIOS"].values():
                with self.subTest(filename=filename):
                    runpy.run_path(str(QA / filename))

    def test_every_scenario_has_help_without_optional_dependencies(self):
        for scenario in self.registry["SCENARIOS"]:
            with self.subTest(scenario=scenario):
                result = subprocess.run(
                    [sys.executable, str(QA), scenario, "--help"],
                    capture_output=True,
                    text=True,
                    timeout=10,
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("usage:", result.stdout)

    def test_drop_height_is_validated_before_native_tooling_is_imported(self):
        body = runpy.run_path(str(QA / "body_smoke.py"))
        with redirect_stderr(io.StringIO()) as output:
            with self.assertRaises(SystemExit) as error:
                body["main"](["123", "drop"])
        self.assertEqual(error.exception.code, 2)
        self.assertIn("requires a height", output.getvalue())


if __name__ == "__main__":
    unittest.main()
