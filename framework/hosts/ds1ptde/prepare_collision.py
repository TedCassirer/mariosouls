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


def invariant_new_londo_collisions(map_id, events, common, parts):
    """Deduplicate reviewed flooded/drained alternatives with identical geometry.

    Event 11600150 switches collision instances as well as navigation groups.
    Identical model/transform/filter pairs describe terrain that remains present
    on both sides of that switch. Different models/transforms stay native-owned.
    Fail closed if the reviewed event program or any other writer changes.
    """
    if map_id != "m16_00_00_00" or 11600150 not in events:
        return set()
    program = events[11600150].instructions
    if len(program) != 83 or any(i.event_arg_replacements for i in program):
        return set()
    if (program[0].category, program[0].index, program[0].args_list) != (
        1003,
        1,
        [63, 1, 0, 11600100],
    ) or (program[32].category, program[32].index, program[32].args_list) != (
        3,
        0,
        [0, 1, 0, 11600100],
    ):
        return set()
    before = set(range(1603200, 1603218))
    after_order = list(range(1603250, 1603268)) + [1603121, 1603122]
    expected = [(i, 0) for i in after_order] + [(i, 1) for i in after_order]
    expected += [(i, 0) for i in sorted(before)]
    actual = [tuple(i.args_list) for i in program if (i.category, i.index) == (2011, 1)]
    # The two extra collision IDs occur after navigation commands in each block.
    if actual != expected:
        return set()
    for index, i in enumerate(program):
        if index not in (0, 32) and (i.category, i.index) not in ((2011, 1), (2012, 1), (2010, 3)):
            return set()
    candidates = [p.entity_id for p in parts]
    unsafe = controlled_collisions(common, candidates) | controlled_collisions(
        {k: v for k, v in events.items() if k != 11600150}, candidates
    )
    by_id = {p.entity_id: p for p in parts if p.entity_id >= 0}

    def geometry(p):
        return (
            p.model.name,
            tuple(p.translate),
            tuple(p.rotate),
            tuple(p.scale),
            int(p.hit_filter_id),
        )

    keep = set()
    for entity in sorted(before):
        alternate = entity + 50
        if entity in unsafe or alternate in unsafe or entity not in by_id or alternate not in by_id:
            continue
        if geometry(by_id[entity]) == geometry(by_id[alternate]):
            keep.add(entity)
    return keep


def prepare(game, map_id, output, native_fallback=False):
    import numpy as np
    from soulstruct.containers import Binder
    from soulstruct.darksouls1ptde.maps import MSB
    from soulstruct.darksouls1ptde.events.emevd import EMEVD
    from soulstruct.utilities.maths import Matrix3
    from soulstruct.havok.fromsoft.shared.map_collision import MapCollisionModel
    from soulstruct.havok.exceptions import TypeNotDefinedError

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
    events = EMEVD.from_bytes(event_bytes).events
    common = EMEVD.from_bytes(archive.read("/event/common.emevd.dcx")).events
    controlled = controlled_collisions(events, [p.entity_id for p in msb.collisions])
    # Common scripts can also contain literal map entity IDs.
    controlled |= controlled_collisions(
        common,
        [p.entity_id for p in msb.collisions],
    )
    invariant = invariant_new_londo_collisions(map_id, events, common, msb.collisions)
    stem = f"/map/{map_id}/h{map_id[1:]}"
    binder = Binder.from_bytes(
        archive.read(stem + ".hkxbhd"), bdt_data=archive.read(stem + ".hkxbdt")
    )
    entries = {Path(e.path.replace("\\", "/")).name.lower(): e for e in binder.entries}
    triangles = []
    filters = []
    native = []
    for part in msb.collisions:
        if part.entity_id in controlled and part.entity_id not in invariant:
            native.append(dict(name=part.name, entity_id=part.entity_id))
            continue
        name = part.model.name.lower() + f"a{map_id[1:3]}.hkx.dcx"
        if name not in entries:
            candidates = [n for n in entries if n.startswith(part.model.name.lower() + ".")]
            if len(candidates) != 1:
                raise ValueError("Missing collision model: " + part.name)
            name = candidates[0]
        try:
            model = MapCollisionModel.from_bytes(entries[name].get_uncompressed_data())
        except TypeNotDefinedError as error:
            if not native_fallback:
                raise
            # Unknown Havok shapes remain in native physics. Never replace them
            # with guessed triangles, a box, or an empty model presented as parsed.
            native.append(
                dict(
                    name=part.name,
                    entity_id=part.entity_id,
                    reason="unsupported-havok-type",
                    detail=str(error),
                )
            )
            print(f"Native-only collision {part.name}: {error}", flush=True)
            continue
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
                invariant_collision_ids=sorted(invariant),
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
    p.add_argument(
        "--native-fallback",
        action="store_true",
        help="Record unsupported Havok shapes as native-only instead of guessing geometry",
    )
    a = p.parse_args()
    prepare(a.game, a.map, a.output, native_fallback=a.native_fallback)
