"""Prepare libsm64's geometry sources from its pinned, checksum-verified inputs.

Transformation follows libsm64/import-mario-geo.py (CC0); no ROM is downloaded.
Generated sources stay in the private build tree.
"""

import hashlib
import json
import runpy
import urllib.request


def prepare_geometry(root, staged):
    lock = json.loads((root / "dependencies.lock.json").read_text())
    upstream = runpy.run_path(str(root / "work/libsm64/import-mario-geo.py"))
    cache = root / "work/runtime-geometry-source"
    cache.mkdir(parents=True, exist_ok=True)
    sources = {}
    expected_urls = {"geo.inc.c": upstream["GEO_URL"], "model.inc.c": upstream["MODEL_URL"]}
    for item in lock["runtime_geometry"]:
        name = item["name"]
        if name not in expected_urls or item["url"] != expected_urls[name]:
            raise RuntimeError("Geometry source differs from pinned importer")
        path = cache / name
        if path.is_file():
            data = path.read_bytes()
        else:
            with urllib.request.urlopen(item["url"], timeout=45) as stream:
                data = stream.read(1024 * 1024 + 1)
        if len(data) > 1024 * 1024 or hashlib.sha256(data).hexdigest() != item["sha256"]:
            raise RuntimeError("Geometry source hash mismatch: " + name)
        if not path.exists():
            with path.open("xb") as stream:
                stream.write(data)
        sources[name] = data.decode("utf-8")
    if set(sources) != set(expected_urls):
        raise RuntimeError("Incomplete runtime geometry inputs")
    header = upstream["model_inc_h"]
    lines = sources["model.inc.c"].splitlines()
    skip = 0
    for i, line in enumerate(lines):
        if skip:
            skip -= 1
            lines[i] = "//" + line
        elif line.startswith("ALIGNED8 static const u8 mario_"):
            skip = 2
            lines[i] = "//" + line
        elif line.startswith("const "):
            header += "\nextern " + line.replace(" = {", ";")
    lines.insert(0, '#include "../../gfx_macros.h"')
    lines.insert(0, '#include "../../load_tex_data.h"')
    destination = staged / "src/decomp/mario"
    destination.mkdir(parents=True, exist_ok=True)
    outputs = {
        "geo.inc.c": upstream["geo_inc_c_header"]
        + sources["geo.inc.c"]
        + upstream["geo_inc_c_footer"],
        "model.inc.c": "\n".join(lines),
        "model.inc.h": header,
        "geo.inc.h": upstream["geo_inc_h"],
    }
    for name, data in outputs.items():
        # Match the Windows upstream importer used by the existing runtime.
        (destination / name).write_text(data, encoding="utf-8", newline="\r\n")
