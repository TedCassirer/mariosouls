"""Edition discovery. Only implemented hosts may build or mutate an installation."""

import json
from pathlib import Path

HOSTS = Path(__file__).resolve().parents[1] / "framework/hosts"


def edition(identifier):
    # Explicit allowlist: a directory or experimental note is not support.
    if identifier != "ds1ptde":
        raise ValueError(
            "Unsupported host edition: "
            + identifier
            + ". Only ds1ptde is implemented; Remastered requires an owned installation."
        )
    return json.loads((HOSTS / identifier / "edition.json").read_text(encoding="utf-8"))
