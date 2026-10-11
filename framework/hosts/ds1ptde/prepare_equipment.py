"""Prepare private rigid sword/shield props from the user's installed PTDE files.

SCPROP1 is little-endian: magic[8], u32 vertex/triangle/width/height counts,
float bounds[min xyz, max xyz], interleaved float position3/normal3/UV2 vertices,
u32 triangle indices, then width*height straight RGBA8 pixels (top row first).
Coordinates are grip-relative metres; sword blades and shield fronts face +Z.
No game archive, parameter, inventory or save is modified.
"""
from __future__ import annotations

import argparse
import hashlib
from io import BytesIO
import json
import os
from pathlib import Path
import shutil
import struct
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'work/soulstruct-source/src'), str(ROOT / 'work/python-deps')]
import numpy as np

MAGIC = b'SCPROP1\0'
HEADER = struct.Struct('<8s4I6f')
MAX_VERTICES = 65536
MAX_TRIANGLES = 65536
MAX_TEXTURE = 2048


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def canonical_matrix(kind):
    if kind == 'sword':
        # Native blades point -Y. -90 degrees around X produces +Z.
        return np.array([[1., 0., 0.], [0., 0., 1.], [0., -1., 0.]])
    if kind == 'shield':
        # Native shield fronts point -Z. Preserve Y-up with 180 degrees around Y.
        return np.diag([-1., 1., -1.])
    raise ValueError('Unsupported equipment kind: ' + str(kind))


def transform_vertices(positions, normals, matrix, kind):
    """Bake a rigid rest transform and canonical axes, with correct normal scaling."""
    positions = np.asarray(positions, dtype=np.float64)
    normals = np.asarray(normals, dtype=np.float64)
    matrix = np.asarray(matrix, dtype=np.float64)
    if (positions.ndim != 2 or positions.shape[1] != 3 or normals.shape != positions.shape
            or matrix.shape != (4, 4) or not np.isfinite(matrix).all()
            or not np.isfinite(positions).all() or not np.isfinite(normals).all()):
        raise ValueError('Invalid rigid vertex transform')
    linear = matrix[:3, :3]
    if abs(np.linalg.det(linear)) < 1e-8:
        raise ValueError('Singular equipment bone transform')
    canonical = canonical_matrix(kind)
    p = (positions @ linear.T + matrix[:3, 3]) @ canonical.T
    n = normals @ np.linalg.inv(linear) @ canonical.T
    lengths = np.linalg.norm(n, axis=1)
    if (lengths < 1e-6).any():
        raise ValueError('Zero equipment vertex normal')
    return p, n / lengths[:, None]


def bone_matrices(bones):
    from soulstruct.utilities.maths import Matrix3
    resolved, visiting = {}, set()

    def resolve(index):
        if index in resolved:
            return resolved[index]
        if index in visiting or not 0 <= index < len(bones):
            raise ValueError('Invalid equipment bone hierarchy')
        visiting.add(index)
        bone = bones[index]
        matrix = np.eye(4)
        matrix[:3, :3] = np.array(Matrix3.from_euler_angles_rad(bone.rotate).data) @ np.diag(list(bone.scale))
        matrix[:3, 3] = list(bone.translate)
        if bone.parent_bone_index != -1:
            matrix = resolve(bone.parent_bone_index) @ matrix
        visiting.remove(index)
        resolved[index] = matrix
        return matrix

    return [resolve(i) for i in range(len(bones))]


def mesh_vertices(mesh, matrices, kind):
    if mesh.is_dynamic or len(mesh.vertex_arrays) != 1:
        raise ValueError('Only verified rigid equipment meshes are supported')
    array = mesh.vertex_arrays[0].array
    if not {'position', 'normal', 'uv_0', 'bone_indices'} <= set(array.dtype.names):
        raise ValueError('Unsupported equipment vertex layout')
    local_indices = array['bone_indices'][:, 0]
    if mesh.bone_indices is None:
        global_indices = local_indices
    else:
        if (local_indices < 0).any() or (local_indices >= len(mesh.bone_indices)).any():
            raise ValueError('Equipment vertex references an invalid local bone')
        global_indices = mesh.bone_indices[local_indices]
    if (global_indices < 0).any() or (global_indices >= len(matrices)).any():
        raise ValueError('Equipment vertex references an invalid bone')
    result = np.empty((len(array), 8), dtype='<f4')
    for bone_index in np.unique(global_indices):
        selected = global_indices == bone_index
        p, n = transform_vertices(array['position'][selected], array['normal'][selected],
                                  matrices[int(bone_index)], kind)
        result[selected, :3] = p
        result[selected, 3:6] = n
    result[:, 6:8] = array['uv_0'][:, :2]
    return result


def write_prop(path, vertices, triangles, width, height, rgba):
    """Validate the bounded format before publishing a prop file."""
    vertices = np.asarray(vertices, dtype='<f4')
    triangles = np.asarray(triangles)
    if (vertices.ndim != 2 or vertices.shape[1] != 8
            or not 3 <= len(vertices) <= MAX_VERTICES or not np.isfinite(vertices).all()):
        raise ValueError('Invalid equipment vertices')
    if (triangles.ndim != 2 or triangles.shape[1] != 3
            or not 1 <= len(triangles) <= MAX_TRIANGLES or not np.issubdtype(triangles.dtype, np.integer)
            or (triangles < 0).any() or (triangles >= len(vertices)).any()):
        raise ValueError('Invalid equipment triangle indices')
    if (not isinstance(width, int) or not isinstance(height, int)
            or not 1 <= width <= MAX_TEXTURE or not 1 <= height <= MAX_TEXTURE
            or len(rgba) != width * height * 4):
        raise ValueError('Invalid equipment RGBA texture')
    lengths = np.linalg.norm(vertices[:, 3:6], axis=1)
    if ((lengths < .99) | (lengths > 1.01)).any():
        raise ValueError('Equipment vertex normals must be unit length')
    minimum, maximum = vertices[:, :3].min(axis=0), vertices[:, :3].max(axis=0)
    if np.abs(vertices[:, :3]).max() >= 5:
        raise ValueError('Equipment bounds exceed supported metre range')
    if np.abs(vertices[:, 6:8]).max() >= 100:
        raise ValueError('Equipment UV coordinates exceed supported range')
    bounds = np.concatenate((minimum, maximum)).tolist()
    path = Path(path)
    with path.open('xb') as stream:
        stream.write(HEADER.pack(MAGIC, len(vertices), len(triangles), width, height, *bounds))
        stream.write(vertices.tobytes())
        stream.write(triangles.astype('<u4').tobytes())
        stream.write(rgba)
    return dict(vertices=len(vertices), triangles=len(triangles), width=width, height=height,
                bounds=bounds, sha256=sha256(path))


def extract_model(archive, model_id, kind, output):
    from PIL import Image
    from soulstruct.containers import Binder, TPF
    from soulstruct.flver import FLVER
    source = f'/parts/WP_A_{model_id:04d}.partsbnd.dcx'
    raw = archive.read(source)
    binder = Binder.from_bytes(raw)
    main_name = f'wp_a_{model_id:04d}.flver'
    main_entries = [e for e in binder.entries if Path(e.path.replace('\\', '/')).name.lower() == main_name]
    if len(main_entries) != 1:
        raise ValueError('Missing or ambiguous main equipment mesh: ' + source)
    flver = FLVER.from_bytes(main_entries[0].get_uncompressed_data())
    matrices = bone_matrices(flver.bones)
    textures = {}
    for entry in binder.entries:
        if entry.path.lower().endswith('.tpf'):
            for texture in TPF.from_bytes(entry.get_uncompressed_data()).textures:
                if texture.stem.lower() in textures:
                    raise ValueError('Duplicate equipment texture name')
                textures[texture.stem.lower()] = texture.data
    vertex_blocks, index_blocks, diffuse_names = [], [], set()
    vertex_offset = 0
    for mesh in flver.meshes:
        diffuse = [t for t in mesh.material.textures if t.texture_type == 'g_Diffuse' and t.path]
        if len(diffuse) != 1:
            raise ValueError('Expected one verified diffuse texture per equipment mesh')
        diffuse_names.add(Path(diffuse[0].path.replace('\\', '/')).stem.lower())
        faces = [s.triangulate(uses_0xffff_separators=True) for s in mesh.face_sets if s.flags == 0]
        if len(faces) != 1:
            raise ValueError('Expected one main non-blur equipment face set')
        faces = faces[0]
        vertices = mesh_vertices(mesh, matrices, kind)
        if faces.size == 0 or faces.max() >= len(vertices):
            raise ValueError('Equipment faces exceed the vertex array')
        # Strip unused LOD vertices, so bounds describe the rendered geometry.
        used, remapped = np.unique(faces, return_inverse=True)
        vertex_blocks.append(vertices[used])
        index_blocks.append(remapped.reshape((-1, 3)) + vertex_offset)
        vertex_offset += len(used)
    if len(diffuse_names) != 1:
        raise ValueError('This prop version requires a single shared diffuse texture')
    diffuse_name = next(iter(diffuse_names))
    if diffuse_name not in textures:
        raise ValueError('Missing equipment diffuse texture: ' + diffuse_name)
    image = Image.open(BytesIO(textures[diffuse_name]))
    if image.format != 'DDS':
        raise ValueError('Expected a PC DDS equipment texture')
    image = image.convert('RGBA')
    target = output / f'wp_a_{model_id:04d}.scprop'
    metadata = write_prop(target, np.concatenate(vertex_blocks), np.concatenate(index_blocks),
                          image.width, image.height, image.tobytes())
    metadata.update(file=target.name, kind=kind, source=source,
                    source_sha256=hashlib.sha256(raw).hexdigest(), diffuse=diffuse_name)
    if kind == 'sword':
        metadata['blade_base'] = [0., 0., 0.]
        metadata['blade_tip'] = [0., 0., metadata['bounds'][5]]
        metadata['blade_length'] = metadata['bounds'][5]
    return metadata


def prepare(game, output):
    from archive import Archives
    from soulstruct.containers import Binder
    from soulstruct.darksouls1ptde.params import GameParamBND
    from soulstruct.darksouls1ptde.text.fmg import FMG
    game, output = Path(game).resolve(), Path(output).resolve()
    if output.exists():
        raise ValueError('Use a fresh output directory; existing caches are immutable')
    archive = Archives(game)
    raw_params = archive.read('/param/GameParam/GameParam.parambnd.dcx')
    params = GameParamBND.from_bytes(raw_params)
    text = Binder.from_bytes(archive.read('/msg/ENGLISH/item.msgbnd.dcx'))
    names = FMG.from_bytes(next(e for e in text.entries if e.entry_id == 11).get_uncompressed_data()).entries
    base_rows, selected_models = {}, {}
    for row_id, row in params.Weapons.rows.items():
        if row_id % 1000 or not names.get(row_id, '').strip():
            continue
        if row.AttackAnimationCategory == 23 and row.WeaponCategory == 1:
            kind = 'sword'
        elif row.AttackAnimationCategory == 48 and row.WeaponCategory == 12:
            kind = 'shield'
        else:
            continue
        if row.WeaponModel in selected_models and selected_models[row.WeaponModel] != kind:
            raise ValueError('A model is shared by incompatible equipment kinds')
        if not 0 <= row.WeaponModel <= 9999:
            raise ValueError('Equipment model ID exceeds the renderer bounds')
        selected_models[row.WeaponModel] = kind
        base_rows[row_id] = kind
    if not 1 <= len(selected_models) <= 64:
        raise ValueError('Equipment model count exceeds the renderer bounds')
    equipment = {}
    for row_id, row in params.Weapons.rows.items():
        base_id = row_id // 1000 * 1000
        if base_id not in base_rows or row.WeaponModel not in selected_models:
            continue
        equipment[str(row_id)] = dict(base_id=base_id, model=row.WeaponModel,
                                      kind=base_rows[base_id], name=names[base_id])
    output.parent.mkdir(parents=True, exist_ok=True)
    staging = Path(tempfile.mkdtemp(prefix=output.name + '.staging-', dir=output.parent)).resolve()
    try:
        models = {str(i): extract_model(archive, i, kind, staging)
                  for i, kind in sorted(selected_models.items())}
        manifest = dict(schema=1, kind='ds1ptde-equipment-props', game_exe_sha256=sha256(game / 'DARKSOULS.exe'),
                        game_params_sha256=hashlib.sha256(raw_params).hexdigest(),
                        coordinate_system='grip-relative metres; blade/front +Z; Y-up; UV unchanged',
                        files={m['file']: m['sha256'] for m in models.values()}, models=models,
                        equipment=equipment)
        (staging / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
        if output.exists():
            raise ValueError('Output appeared during preparation; refusing to replace it')
        os.rename(staging, output)
    except BaseException:
        # Only remove the freshly created, verified sibling staging directory.
        if staging.parent == output.parent and staging.name.startswith(output.name + '.staging-') and staging.exists():
            shutil.rmtree(staging)
        raise
    print(f'Prepared {len(models)} private props for {len(equipment)} equipment rows: {output}')
    return manifest


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    prepare(args.game, args.output)
