"""Implemented edition build/asset dispatch. Installers keep their own edition guards."""

import json
import re
import importlib.util
from pathlib import Path

HOSTS = Path(__file__).resolve().parents[1] / "framework/hosts"


def edition(identifier):
    if not re.fullmatch(r"[a-z][a-z0-9_]{0,46}", identifier):
        raise ValueError("Invalid host edition identifier")
    path = HOSTS / identifier / "edition.json"
    if not path.is_file():
        raise ValueError(
            "Unsupported host edition: "
            + identifier
            + ". No implemented edition declaration; Remastered is not implemented."
        )
    info = json.loads(path.read_text(encoding="utf-8"))
    if info.get("id") != identifier or info.get("status") != "implemented":
        raise ValueError("Host is not declared implemented: " + identifier)
    if info.get("target") not in ("x86-windows-gnu", "x86_64-windows-gnu"):
        raise ValueError("Unsupported host target: " + identifier)
    if not re.fullmatch(r"[0-9a-f]{64}", info.get("supported_exe_sha256", "")):
        raise ValueError("Host requires an edition-specific executable fingerprint")
    recipe(info, "build_recipe")
    output = info.get("build_output")
    if not isinstance(output, str) or not output.endswith(".dll") or Path(output).name != output:
        raise ValueError("Host build output must be a filename")
    return info


def recipe(info, key):
    folder = (HOSTS / info["id"]).resolve()
    name = info.get(key)
    if not isinstance(name, str) or not name:
        raise ValueError("Host recipe missing: " + key)
    path = (folder / name).resolve()
    if not path.is_relative_to(folder) or not path.is_file():
        raise ValueError("Invalid host recipe: " + key)
    return path


def build_host(info, api, output):
    path = recipe(info, "build_recipe")
    spec = importlib.util.spec_from_file_location("host_build_" + info["id"], path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.build(api, output, info["target"])
