"""Unknown editions must fail before build/install or game access."""

import sys
import unittest
from unittest.mock import patch
import framework as f
from host_editions import edition


class Editions(unittest.TestCase):
    def test_unknown_edition_has_no_side_effects(self):
        for command in ("build", "doctor", "test", "install", "uninstall"):
            with (
                self.subTest(command=command),
                patch.object(sys, "argv", ["framework.py", command, "--host", "ds1remastered"]),
                patch.object(f, "run") as run,
                patch.object(f, "build") as build,
            ):
                with self.assertRaisesRegex(ValueError, "Only ds1ptde"):
                    f.main()
                run.assert_not_called()
                build.assert_not_called()

    def test_native_host_remains_32_bit(self):
        host = edition("ds1ptde")
        self.assertEqual(host["target"], "x86-windows-gnu")
        self.assertEqual(host["executable"], "DARKSOULS.exe")

    def test_host_headers_forbidden_in_shared_code(self):
        import tempfile
        from pathlib import Path

        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            shared = root / "framework/core"
            shared.mkdir(parents=True)
            (shared / "bad.h").write_text("#include <d3d11.h>\n")
            with patch.object(f, "ROOT", root), patch.object(f, "FRAMEWORK", root / "framework"):
                with self.assertRaisesRegex(RuntimeError, "host dependency"):
                    f.architecture_check()


class PortableAssets(unittest.TestCase):
    def test_missing_owned_assets_fail_before_building(self):
        from framework_portability import check_owned_character

        with patch.object(f, "compile_cpp") as compile_cpp:
            with self.assertRaisesRegex(ValueError, "owned --asset"):
                check_owned_character("mario64", None, None)
            compile_cpp.assert_not_called()

    def test_unknown_character_cannot_select_output_paths(self):
        from framework_portability import check_owned_character

        with patch.object(f, "compile_cpp") as compile_cpp:
            with self.assertRaises(ValueError):
                check_owned_character("../../untrusted", "asset", "runtime")
            compile_cpp.assert_not_called()

    def test_portable_assets_cli_does_not_dispatch_host_tests(self):
        import framework_tests
        import framework_portability

        with (
            patch.object(
                sys,
                "argv",
                [
                    "framework.py",
                    "test",
                    "--portable-assets",
                    "--asset",
                    "owned",
                    "--runtime",
                    "x64-runtime",
                ],
            ),
            patch.object(framework_portability, "check_owned_character") as check,
            patch.object(framework_tests, "test") as native,
        ):
            f.main()
        check.assert_called_once_with("mario64", "owned", "x64-runtime")
        native.assert_not_called()
