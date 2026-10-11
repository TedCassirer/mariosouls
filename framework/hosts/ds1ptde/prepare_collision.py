"""Export static guest geometry; event-controlled colliders stay native-owned.

Uses the user's installed MSB/Havok/event data. No game-derived data goes in Git.
Unknown script targets remain native-owned by excluding every named candidate.
"""

from pathlib import Path
import argparse
import hashlib
import json
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / "work/soulstruct-source/src"), str(ROOT / "work/python-deps")]
from archive import Archives


def controlled_collisions(events, possible_ids=None):
    ids = set()
    for event in events.values():
        for instruction in event.instructions:
            if instruction.category == 2011 and instruction.index in (1, 2):
                if any(r.arg_index == 0 for r in instruction.event_arg_replacements):
                    if possible_ids is None:
                        raise ValueError(
                            "Resolve parameterized native collision ID before exporting this map"
                        )
                    # A script argument can address any named collision. Keep all
                    # candidates native-owned rather than baking an unknown state.
                    ids.update(i for i in possible_ids if i >= 0)
                    continue
                ids.add(int(instruction.args_list[0]))
    return ids


def prepare(game, map_id, output):
    import numpy as np
    from soulstruct.containers import Binder
    from soulstruct.darksouls1ptde.maps import MSB
    from soulstruct.darksouls1ptde.events.emevd import EMEVD
    from soulstruct.utilities.maths import Matrix3
    from soulstruct.havok.fromsoft.shared.map_collision import MapCollisionModel

    if not re.fullmatch(r"m\d{2}_\d{2}_\d{2}_\d{2}", map_id):
        raise ValueError("Invalid map ID")
    game = Path(game)
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    if (output / "manifest.json").exists():
        raise ValueError("Use a fresh output directory to preserve earlier caches")
    archive = Archives(game)
    msb = MSB.from_bytes(archive.read(f"/map/MapStudio/{map_id}.msb"))
    event_bytes = archive.read(f"/event/{map_id}.emevd.dcx")
    controlled = controlled_collisions(
        EMEVD.from_bytes(event_bytes).events, [p.entity_id for p in msb.collisions]
    )
    # Common scripts can also contain literal map entity IDs.
    controlled |= controlled_collisions(
        EMEVD.from_bytes(archive.read("/event/common.emevd.dcx")).events,
        [p.entity_id for p in msb.collisions],
    )
    stem = f"/map/{map_id}/h{map_id[1:]}"
    binder = Binder.from_bytes(
        archive.read(stem + ".hkxbhd"), bdt_data=archive.read(stem + ".hkxbdt")
    )
    entries = {Path(e.path.replace("\\", "/")).name.lower(): e for e in binder.entries}
    triangles = []
    filters = []
    native = []
    for part in msb.collisions:
        if part.entity_id in controlled:
            native.append(dict(name=part.name, entity_id=part.entity_id))
            continue
        name = part.model.name.lower() + f"a{map_id[1:3]}.hkx.dcx"
        if name not in entries:
            candidates = [n for n in entries if n.startswith(part.model.name.lower() + ".")]
            if len(candidates) != 1:
                raise ValueError("Missing collision model: " + part.name)
            name = candidates[0]
        model = MapCollisionModel.from_bytes(entries[name].get_uncompressed_data())
        rot = np.array(Matrix3.from_euler_angles_deg(part.rotate).data)
        offset = np.array(list(part.translate))
        for mesh in model.meshes:
            tri = (mesh.vertices3D @ rot.T + offset)[mesh.face_vertex_indices].astype("<f4")
            triangles.append(tri)
            filters.extend([int(part.hit_filter_id)] * len(tri))
    data = np.concatenate(triangles)
    if not np.isfinite(data).all() or not 0 < len(data) < 2000000:
        raise ValueError("Invalid collision geometry")
    records = np.empty(len(data), dtype=[("filter", "<u4"), ("triangle", "<f4", (3, 3))])
    records["filter"] = filters
    records["triangle"] = data
    target = output / (map_id + ".bin")
    target.write_bytes(b"MSC2" + struct.pack("<I", len(data)) + records.tobytes())

    def sha(path):
        return hashlib.sha256(Path(path).read_bytes()).hexdigest()

    manifest = dict(
        schema=1,
        policy="event-colliders-native",
        game_exe_sha256=sha(game / "DARKSOULS.exe"),
        files={target.name: sha(target)},
        maps={
            map_id: dict(
                triangles=len(data),
                hit_filters={str(i): filters.count(i) for i in set(filters)},
                native_owned=native,
            )
        },
    )
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2))
    print(
        f"Exported {len(data)} triangles; {len(native)} event-controlled colliders remain native-owned: {native}"
    )


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--game", required=True)
    p.add_argument("--map", required=True)
    p.add_argument("--output", required=True)
    a = p.parse_args()
    prepare(a.game, a.map, a.output)
