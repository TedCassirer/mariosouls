"""Render private SDK equipment dumps without launching or controlling a game.

Reads the same mesh and glove sockets as the host, plus private SCPROP1 assets.
The output is visual QA, not evidence of native damage or in-game performance.
"""

from pathlib import Path
import argparse
import struct
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "work/python-deps"))
import numpy as np
from PIL import Image, ImageDraw

NAMES = {
    1: "Longsword",
    3: "Dagger",
    4: "Scimitar",
    5: "Claymore",
    6: "Battle Axe",
    7: "Mace",
    8: "Pyromancy (right)",
    9: "Pyromancy (left)",
}
MODELS = {1: 201, 3: 100, 4: 400, 5: 266, 6: 505, 7: 606}


def read_dump(path):
    frames = []
    with Path(path).open("rb") as stream:
        magic, w, h, count = struct.unpack("<8s3I", stream.read(20))
        if magic != b"SCPOSE1\0" or not 0 < count < 10000:
            raise ValueError("Bad preview dump")
        texture = np.frombuffer(stream.read(w * h * 4), np.uint8).reshape(h, w, 4)

        def floats(n):
            return np.frombuffer(stream.read(n * 4), "<f4")

        for _ in range(count):
            kind, combo, frame, n = struct.unpack("<4I", stream.read(16))
            phase = float(floats(1)[0])
            hands = [floats(16).reshape(4, 4).T for _ in range(2)]
            p, normal, color = [floats(n * 9).reshape(-1, 3) for _ in range(3)]
            uv = floats(n * 6).reshape(-1, 2)
            frames.append(
                dict(
                    kind=kind,
                    combo=combo,
                    frame=frame,
                    phase=phase,
                    hands=hands,
                    p=p,
                    normal=normal,
                    color=color,
                    uv=uv,
                )
            )
        if stream.read(1):
            raise ValueError("Trailing dump bytes")
    return texture, frames


def read_prop(path):
    b = Path(path).read_bytes()
    magic, nv, nt, w, h, *bounds = struct.unpack_from("<8s4I6f", b)
    if magic != b"SCPROP1\0":
        raise ValueError("Bad prop")
    v = np.frombuffer(b, "<f4", nv * 8, 48).reshape(-1, 8)
    faces = np.frombuffer(b, "<u4", nt * 3, 48 + nv * 32)
    texture = np.frombuffer(b, np.uint8, w * h * 4, 48 + nv * 32 + nt * 12).reshape(h, w, 4)
    return v[faces], texture


def render(frame, texture, props, size=400, yaw=35):
    yaw = np.deg2rad(yaw)
    elev = 0.15
    eye = np.array([np.sin(yaw), elev, np.cos(yaw)])
    eye /= np.linalg.norm(eye)
    right = np.cross(eye, [0, 1, 0])
    right /= np.linalg.norm(right)
    up = np.cross(right, eye)
    basis = np.array([right, up, eye])
    scale = size / 2.8
    center = np.array([0, 0.80, 0])
    image = np.zeros((size, size, 3), np.uint8) + [24, 29, 36]
    depth = np.full((size, size), -np.inf)
    light = np.array([-0.4, 0.8, 0.6])
    light /= np.linalg.norm(light)

    def draw(p, n, c, uv, tex, mario=False):
        camera = (p - center) @ basis.T
        screen = camera[:, :2] * [scale, -scale] + [size * 0.5, size * 0.56]
        for ids in np.arange(len(p)).reshape(-1, 3):
            tri = screen[ids]
            z = camera[ids, 2]
            lo = np.maximum(np.floor(tri.min(0)).astype(int), 0)
            hi = np.minimum(np.ceil(tri.max(0)).astype(int), size - 1)
            if (hi < lo).any():
                continue
            xx, yy = np.meshgrid(
                np.arange(lo[0], hi[0] + 1) + 0.5, np.arange(lo[1], hi[1] + 1) + 0.5
            )
            den = (tri[1, 1] - tri[2, 1]) * (tri[0, 0] - tri[2, 0]) + (tri[2, 0] - tri[1, 0]) * (
                tri[0, 1] - tri[2, 1]
            )
            if abs(den) < 1e-8:
                continue
            a = (
                (tri[1, 1] - tri[2, 1]) * (xx - tri[2, 0])
                + (tri[2, 0] - tri[1, 0]) * (yy - tri[2, 1])
            ) / den
            b = (
                (tri[2, 1] - tri[0, 1]) * (xx - tri[2, 0])
                + (tri[0, 0] - tri[2, 0]) * (yy - tri[2, 1])
            ) / den
            weights = np.stack([a, b, 1 - a - b], -1)
            zz = weights @ z
            region = depth[lo[1] : hi[1] + 1, lo[0] : hi[0] + 1]
            mask = (weights.min(-1) >= 0) & (zz > region)
            if not mask.any():
                continue
            texuv = np.clip(weights @ uv[ids], 0, 1)
            tx = (texuv[:, :, 0] * (tex.shape[1] - 1)).astype(int)
            ty = (texuv[:, :, 1] * (tex.shape[0] - 1)).astype(int)
            pixel = tex[ty, tx] / 255.0
            alpha = pixel[:, :, 3, None]
            rgb = weights @ c[ids]
            if mario:
                rgb = rgb * (1 - alpha) + pixel[:, :, :3] * alpha
            else:
                rgb = pixel[:, :, :3]
                mask &= pixel[:, :, 3] >= 0.5
            shade = 0.48 + 0.52 * np.maximum(0, (weights @ n[ids]) @ light)
            image[lo[1] : hi[1] + 1, lo[0] : hi[0] + 1][mask] = np.clip(
                rgb * shade[:, :, None] * 255, 0, 255
            ).astype(np.uint8)[mask]
            region[mask] = zz[mask]

    # Actual guest mesh; host model scaling is 0.9 and units are centimetres.
    draw(frame["p"] * 0.009, frame["normal"], frame["color"], frame["uv"], texture, True)
    for side, model in [(0, MODELS[frame["kind"]]), (1, 1523)] if frame["kind"] in MODELS else []:
        if model not in props:
            continue
        v, tex = props[model]
        m = frame["hands"][side]
        p = v[:, :3] @ m[:3, :3].T * 0.9 + m[:3, 3] * 0.009
        n = v[:, 3:6] @ m[:3, :3].T
        draw(p, n, np.ones_like(p), v[:, 6:8], tex)
    out = Image.fromarray(image.astype(np.uint8))
    label = ImageDraw.Draw(out)
    label.text(
        (12, 10),
        f"{NAMES[frame['kind']]}  /  swing {frame['combo']}  /  {frame['phase']:.2f}",
        fill=(225, 232, 240),
    )
    return out


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("dump")
    p.add_argument("--props", required=True)
    p.add_argument("--sheet", required=True)
    p.add_argument("--frames")
    p.add_argument("--size", type=int, default=320)
    p.add_argument("--yaw", type=float, default=35)
    args = p.parse_args()
    texture, frames = read_dump(args.dump)
    directory = Path(args.props)
    props = {
        m: read_prop(directory / f"wp_a_{m:04d}.scprop")
        for m in [*MODELS.values(), 1523]
        if (directory / f"wp_a_{m:04d}.scprop").exists()
    }
    kinds = [k for k in NAMES if any(f["kind"] == k for f in frames)]
    size = args.size
    sheet = Image.new("RGB", (size * 5, size * len(kinds)))
    phases = [0, 0.20, 0.37, 0.55, 0.85]
    for row, kind in enumerate(kinds):
        available = [f for f in frames if f["kind"] == kind and f["combo"] == 1]
        for col, phase in enumerate(phases):
            frame = min(available, key=lambda f: abs(f["phase"] - phase))
            sheet.paste(render(frame, texture, props, size, args.yaw), (col * size, row * size))
    sheet.save(args.sheet)
    if args.frames:
        output = Path(args.frames)
        output.mkdir(parents=True, exist_ok=False)
        for i, frame in enumerate(frames):
            render(frame, texture, props, args.size, args.yaw).save(output / f"{i:04d}.png")
            if i % 60 == 0:
                print(f"Rendered {i}/{len(frames)}", flush=True)


if __name__ == "__main__":
    main()
