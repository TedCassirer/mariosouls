"""Build recipes expose discovery without starting dependency or compiler work."""

from contextlib import redirect_stdout
import io
from pathlib import Path
import runpy
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
MARIO = ROOT / "framework/characters/mario64"


class RuntimeRecipe(unittest.TestCase):
    def load(self):
        with patch.object(sys, "path", [str(MARIO), *sys.path]):
            return runpy.run_path(str(MARIO / "build_runtime.py"))

    def test_import_does_not_prepare_sources_or_launch_a_process(self):
        with patch("subprocess.run") as launch, patch("shutil.copytree") as copy:
            self.load()
        launch.assert_not_called()
        copy.assert_not_called()

    def test_help_does_not_start_a_build(self):
        recipe = self.load()
        with patch("subprocess.run") as launch, redirect_stdout(io.StringIO()) as output:
            with self.assertRaises(SystemExit) as result:
                recipe["main"](["--help"])
        self.assertEqual(result.exception.code, 0)
        self.assertIn("--output", output.getvalue())
        launch.assert_not_called()

    def test_cli_returns_the_compiler_failure_status(self):
        recipe = self.load()
        with patch.dict(recipe["main"].__globals__, build_runtime=lambda output: 9):
            self.assertEqual(recipe["main"](["--output", "private-build"]), 9)


if __name__ == "__main__":
    unittest.main()
