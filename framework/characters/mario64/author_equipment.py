"""Fit original weapon key poses to the user's owned Mario skeleton.

Only authored joint keys are emitted; no ROM animation/mesh is copied. This is
an offline authoring tool, not a gameplay dependency. Requires numpy/scipy.
"""

from pathlib import Path
import argparse
import math
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "work/python-deps"))
import numpy as np
from scipy.optimize import least_squares

PARENTS = [-1, 0, 1, 2, 2, 4, 5, 6]
OFFSETS = [
    [0, 0, 0],
    [0, 0, 0],
    [68, 0, 0],
    [87, 0, 0],
    [67, -10, 79],
    [0, 0, 0],
    [65, 0, 0],
    [60, 0, 0],
]


def matrix(t, a):
    sx, sy, sz = np.sin(a)
    cx, cy, cz = np.cos(a)
    return np.array(
        [
            [cy * cz, cy * sz, -sy, 0],
            [sx * sy * cz - cx * sz, sx * sy * sz + cx * cz, sx * cy, 0],
            [cx * sy * cz + sx * sz, cx * sy * sz - sx * cz, cx * cy, 0],
            [*t, 1.0],
        ]
    )


def fk(p):
    nodes = []
    for i in range(8):
        m = matrix(
            np.array(OFFSETS[i]) + (p[:3] if i == 0 else 0),
            p[3 + i * 3 : 6 + i * 3] * math.pi / 32768,
        )
        nodes.append(m if PARENTS[i] < 0 else m @ nodes[PARENTS[i]])
    return nodes


def idle(rom):
    raw = Path(rom).read_bytes()
    base = 0x4EC000
    off, _ = struct.unpack_from(">II", raw, base + 8 + 0xC3 * 8)
    a = base + off
    *_, values, index, _ = struct.unpack_from(">6hIII", raw, a)
    return np.array(
        [
            struct.unpack_from(
                ">h", raw, a + values + struct.unpack_from(">HH", raw, a + index + i * 4)[1] * 2
            )[0]
            for i in range(63)
        ],
        float,
    )


# Grip targets use original skeleton units (4 units = 1 cm). The three active
# keys form a deliberate weapon arc; torso values are authored local XYZ deltas.
# Each entry: right-glove target, weapon direction, torso twist and lean degrees.
REST = ([130, 260, 90], [0.15, 0.75, 0.65], 0, 0)
SLASH = [
    REST,
    ([170, 320, 5], [0.88, 0.22, -0.42], -25, -5),
    ([85, 280, 120], [0.05, 0.02, 1], 0, 4),
    ([20, 240, 100], [-0.87, -0.22, 0.45], 25, 7),
    REST,
]
BACK = [
    REST,
    ([40, 290, 115], [-0.92, 0.12, 0.37], 22, -3),
    ([85, 280, 120], [0.05, -0.03, 1], 0, 4),
    ([185, 245, 65], [0.94, -0.22, 0.28], -22, 5),
    REST,
]
CHOP = [
    REST,
    ([115, 390, 0], [0.12, 0.88, -0.46], -22, -9),
    ([85, 280, 120], [0.08, -0.08, 1], 4, 9),
    ([55, 195, 120], [-0.30, -0.65, 0.70], 20, 13),
    REST,
]
CHOP2 = [
    REST,
    ([180, 360, 5], [0.65, 0.67, -0.36], -30, -6),
    ([85, 275, 120], [0, -0.12, 1], 3, 8),
    ([20, 230, 120], [-0.72, -0.40, 0.58], 28, 10),
    REST,
]


def poses(kind, combo):
    import copy

    source = (CHOP if combo == 1 else CHOP2) if kind >= 6 else (SLASH if combo == 1 else BACK)
    out = copy.deepcopy(source)
    if kind == 3:  # compact, close-guard dagger cuts
        for i in (1, 2, 3):
            grip, direction, twist, lean = out[i]
            grip[0] = 110 + (grip[0] - 110) * 0.72
            grip[1] = 275 + (grip[1] - 275) * 0.7
            grip[2] = 90 + (grip[2] - 90) * 0.8
            out[i] = (grip, direction, twist * 0.6, lean * 0.6)
    elif kind == 4:  # wider flowing curved-sword slash
        for i in (1, 3):
            grip, direction, twist, lean = out[i]
            out[i] = (grip, direction, twist * 1.15, lean)
    elif kind == 5:
        out = copy.deepcopy(CHOP if combo == 1 else BACK)
        for i in (1, 2, 3):
            grip, direction, twist, lean = out[i]
            out[i] = (grip, direction, twist * 1.25, lean)
    elif kind == 7:  # vertical mace impact, less cross-body than the axe
        out = copy.deepcopy(CHOP if combo == 1 else CHOP2)
        out[3] = ([80, 192, 125], [0.05, -0.70, 0.71], 12, 14)
    return out


def fit(base, key, seed):
    target, direction, twist, lean = key
    p = base.copy()
    body = np.array([-twist, 0, lean]) * 32768 / 180
    p[9:12] += body
    forward = np.array(direction, float)
    forward /= np.linalg.norm(forward)
    up = np.array([0, 1, 0.0])
    up -= forward * np.dot(up, forward)
    up /= np.linalg.norm(up)
    # Project the authored glove goal onto a physically reachable wrist sphere,
    # taking the glove's 40-unit grip offset and requested orientation into account.
    # This keeps a bend in the elbow instead of stretching the mesh to the goal.
    hand_x = -np.cross(up, forward)
    shoulder = fk(p)[5][3, :3]
    wrist = np.array(target) - 40 * hand_x
    delta = wrist - shoulder
    distance = np.linalg.norm(delta)
    if distance > 117:
        wrist = shoulder + delta * (117 / distance)
    target = wrist + 40 * hand_x

    def evaluate(a):
        arm = np.array([*a[:3], 0, 0, a[3], *a[4:]])
        p[18:27] = arm * 32768 / math.pi
        n = fk(p)
        m = n[7]
        return arm, n, (np.array([40, 0, 0, 1.0]) @ m)[:3], m[2, :3], -m[1, :3]

    def residual(a):
        arm, n, grip, z, y = evaluate(a)
        wrap = (a - seed + math.pi) % (2 * math.pi) - math.pi
        return np.r_[
            (grip - target) / 18,
            (z - forward) * 5,
            (y - up) * 2,
            0.04 * wrap,
            max(0, 35 - n[6][3, 0]) / 80,
        ]

    result = least_squares(residual, seed, max_nfev=1600)
    arm, n, grip, z, y = evaluate(result.x)
    error = np.linalg.norm(grip - target) * 0.25
    if error > 2 or np.dot(z, forward) < 0.98:
        raise ValueError(f"Unfitted pose: grip error {error:.2f}cm")
    angles = ((np.r_[body, arm * 32768 / math.pi] + 32768) % 65536 - 32768).round().astype(int)
    return angles, result.x, error


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    base = idle(args.rom)
    seed = np.array([-6463, -7897, -30470, 37, -26207, 7206, 26008]) * math.pi / 32768
    tables = []
    maximum = 0
    for kind in (1, 3, 4, 5, 6, 7):
        combos = []
        for combo in (1, 2):
            values = []
            previous = seed.copy()
            for key in poses(kind, combo):
                angle, previous, error = fit(base, key, previous)
                values.append(angle)
                maximum = max(maximum, error)
            # Exactly shared ready pose avoids a loop/second-swing seam.
            values[-1] = values[0].copy()
            combos.append(values)
        tables.append(combos)
    lines = [
        "/* Original authored weapon poses fitted with author_equipment.py. */",
        "/* Key layout: torso XYZ delta, upper-arm XYZ, elbow XYZ, wrist XYZ. */",
        "static const s16 scWeaponKeys[6][2][5][12]={",
    ]
    for kind, combos in zip((1, 3, 4, 5, 6, 7), tables):
        lines.append(" { /* SDK kind " + str(kind) + " */")
        for values in combos:
            lines.append("  {")
            lines.extend("   {" + ",".join(map(str, row)) + "}," for row in values)
            lines.append("  },")
        lines.append(" },")
    lines.append("};")
    lines.append("static const s16 scCriticalKeys[6][5][12]={")
    thrust = [
        REST,
        ([175, 270, 20], [0, 0, 1], -18, -4),
        ([80, 260, 185], [0, 0, 1], 15, 9),
        ([140, 270, 45], [0.05, 0.3, 0.95], -7, 0),
        REST,
    ]
    for kind in (1, 3, 4, 5, 6, 7):
        previous = seed.copy()
        values = []
        for key in thrust if kind < 6 else CHOP:
            angle, previous, error = fit(base, key, previous)
            values.append(angle)
            maximum = max(maximum, error)
        values[-1] = values[0].copy()
        lines.append(" {")
        lines.extend("  {" + ",".join(map(str, row)) + "}," for row in values)
        lines.append(" },")
    lines.append("};")
    Path(args.output).write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Authored 12 swings and 6 criticals; largest glove-target error {maximum:.2f} cm")


if __name__ == "__main__":
    main()
