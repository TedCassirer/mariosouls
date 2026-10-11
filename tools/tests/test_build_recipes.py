"""Build recipes expose discovery without starting dependency or compiler work."""

from contextlib import redirect_stdout
import builtins
import io
from pathlib import Path
import runpy
import sys
import tempfile
import unittest
from unittest.mock import patch
import framework

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


class AssetRecipes(unittest.TestCase):
    def test_adapter_recipe_receives_paths_as_separate_arguments(self):
        with tempfile.TemporaryDirectory() as directory:
            folder = Path(directory)
            for command in ("prepare-assets", "preview-assets"):
                filename = command.replace("-", "_") + ".py"
                (folder / filename).touch()
                with (
                    patch.object(framework, "load_profile", return_value=(folder, {})),
                    patch.object(framework, "run") as run,
                ):
                    framework.asset_recipe(command, "link_ww", "my assets/pack", "private/preview")
                run.assert_called_once_with(
                    [
                        sys.executable,
                        folder / filename,
                        "--input",
                        "my assets/pack",
                        "--output",
                        "private/preview",
                    ]
                )

    def test_missing_recipe_fails_without_launching_a_process(self):
        with (
            tempfile.TemporaryDirectory() as directory,
            patch.object(framework, "load_profile", return_value=(Path(directory), {})),
            patch.object(framework, "run") as run,
        ):
            with self.assertRaisesRegex(ValueError, "no preview-assets recipe"):
                framework.asset_recipe("preview-assets", "mario64", "input", "output")
        run.assert_not_called()

    def test_preview_import_and_help_do_not_load_optional_dependencies(self):
        ordinary_import = builtins.__import__

        def guarded_import(name, *args, **kwargs):
            if name.split(".")[0] in {"numpy", "PIL"}:
                self.fail("Preview discovery imported optional dependency: " + name)
            return ordinary_import(name, *args, **kwargs)

        with patch("builtins.__import__", side_effect=guarded_import):
            recipe = runpy.run_path(str(ROOT / "framework/characters/link_ww/preview_assets.py"))
            with redirect_stdout(io.StringIO()) as output:
                with self.assertRaises(SystemExit) as result:
                    recipe["main"](["--help"])
        self.assertEqual(result.exception.code, 0)
        self.assertIn("--input", output.getvalue())


if __name__ == "__main__":
    unittest.main()
