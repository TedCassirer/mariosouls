"""Compatibility import for the PTDE-owned uninstall implementation."""

import importlib.util
from pathlib import Path
import sys

_path = Path(__file__).resolve().parents[1] / "framework/hosts/ds1ptde/uninstall.py"
_spec = importlib.util.spec_from_file_location(__name__, _path)
_module = importlib.util.module_from_spec(_spec)
sys.modules[__name__] = _module
_spec.loader.exec_module(_module)
