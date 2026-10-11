"""Focused equipment observations on Mario QA. Never grants items or edits HP.

Run after loading Mario QA with a one-handed straight sword and small shield.
`safe` checks hold/release/jump. `front` and `rear` observe an armed hollow and
retreat before HP falls below 200. These stages record evidence, not blanket PASS.
"""

from pathlib import Path
import sys
import time
import json
import math
from host import Host, GAME
from bridge import command
from um.win import Drive, shot

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "work/equipment-qa"
OUT.mkdir(parents=True, exist_ok=True)
pid = int(sys.argv[1])
stage = sys.argv[2]
h = Host(pid, write=True)
d = Drive("DARKSOULS")
rows = []
logstart = (GAME / "mariosouls.log").stat().st_size


def sample(label, target=1810103):
    s = h.snapshot(target)
    assert s["focus"] and s["player"]["hp"] > 0
    a = h.chain(int(s["player"]["reference"], 16), 0x28, 0x2C)
    s["guard_byte"] = h.read(a + 0x80, 1)[0]
    s["guard_disabled"] = h.u(a + 0x17C)
    s["native_guard"] = bool(s["guard_byte"] & 2) and not bool(s["guard_disabled"] & 8)
    s["native_active"] = h.read(a + 0x3C, 1)[0]
    s["label"] = label
    rows.append(s)
    return s


def observe(label, seconds):
    end = time.monotonic() + seconds
    out = []
    while time.monotonic() < end:
        s = sample(label)
        out.append(s)
        if s["player"]["hp"] < 200:
            break
        time.sleep(0.04)
    return out


def at(xyz, yaw=0):
    command(pid, mode=2)
    time.sleep(0.6)
    h.warp(xyz, yaw)
    assert h.focused()
    d.cmd("hold 0x4F 140", retry=False)
    time.sleep(0.7)
    command(pid, mode=1)
    observe("activate", 0.8)


try:
    start = sample("start")
    assert start["equipment"]["stance"] == 1
    if stage == "safe":
        at([3.1, 196.5, 5.7], math.pi)
        command(pid, frames=90, buttons=8)
        guard = observe("guard", 1.4)
        shot(str(OUT / "shield-guard.png"), exe="DARKSOULS.exe", scale=0.5)
        command(pid)
        released = observe("release", 0.7)
        command(pid, frames=8, buttons=9)
        jump = observe("guard_jump", 2.5)
        assert any(s["native_guard"] for s in guard), "Native guard never became active"
        assert not released[-1]["native_guard"], "Guard survived release"
        assert all(s["equipment"]["stance"] == 1 for s in rows), "Guard changed handedness"
        assert (
            max(s["player"]["position"][1] for s in jump) - released[-1]["player"]["position"][1]
            > 2
        )
        assert min(s["player"]["stamina"] for s in jump) >= released[-1]["player"]["stamina"]
    elif stage in ("front", "rear"):
        target = start["target"]
        assert target and target["hp"] > 0
        x, y, z = target["position"]
        at([x, y + 0.2, z + 1.3], 0 if stage == "front" else math.pi)
        command(pid, frames=150, buttons=8)
        observe(stage, 4.8)
        shot(str(OUT / (stage + "-block.png")), exe="DARKSOULS.exe", scale=0.5)
    else:
        raise ValueError("Unknown stage")
finally:
    try:
        command(pid)
        if stage in ("front", "rear"):
            command(pid, mode=2)
            time.sleep(0.6)
            h.warp([3.1, 196.5, 5.7], math.pi)
            time.sleep(0.7)
            command(pid, mode=1)
    finally:
        (OUT / (stage + "-trace.json")).write_text(json.dumps(rows, indent=2))
        log = (GAME / "mariosouls.log").read_bytes()[logstart:]
        (OUT / (stage + "-log.txt")).write_bytes(log)
        d.close()
        h.close()
print(
    json.dumps(
        dict(
            stage=stage,
            samples=len(rows),
            hp=[rows[0]["player"]["hp"], rows[-1]["player"]["hp"]],
            stamina_min=min(s["player"]["stamina"] for s in rows),
            guard_samples=sum(s["native_guard"] for s in rows),
            passive_samples=sum(not s["native_active"] for s in rows),
        )
    )
)
