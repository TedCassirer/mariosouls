"""Focused equipment observations on Mario QA. Never grants items or edits HP.

Run after loading Mario QA with a one-handed straight sword and small shield.
`safe` checks hold/release/jump. `front` and `rear` observe an armed hollow and
retreat before HP falls below 200. These stages record evidence, not blanket PASS.
"""

from pathlib import Path
import time
import json
import math
import argparse


def main(arguments=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pid", type=int)
    parser.add_argument("stage", choices=["safe", "front", "rear"])
    parser.add_argument(
        "--output", type=Path, default=Path(__file__).resolve().parents[2] / "work/equipment-qa"
    )
    args = parser.parse_args(arguments)

    from host import Host, GAME
    from bridge import command
    from um.win import Drive, shot

    output = args.output
    output.mkdir(parents=True, exist_ok=True)
    pid = args.pid
    stage = args.stage
    host = Host(pid, write=True)
    driver = Drive("DARKSOULS")
    rows = []
    logstart = (GAME / "mariosouls.log").stat().st_size

    def sample(label, target=1810103):
        snapshot = host.snapshot(target)
        assert snapshot["focus"] and snapshot["player"]["hp"] > 0
        a = host.chain(int(snapshot["player"]["reference"], 16), 0x28, 0x2C)
        snapshot["guard_byte"] = host.read(a + 0x80, 1)[0]
        snapshot["guard_disabled"] = host.u(a + 0x17C)
        snapshot["native_guard"] = bool(snapshot["guard_byte"] & 2) and not bool(
            snapshot["guard_disabled"] & 8
        )
        snapshot["native_active"] = host.read(a + 0x3C, 1)[0]
        snapshot["label"] = label
        rows.append(snapshot)
        return snapshot

    def observe(label, seconds):
        end = time.monotonic() + seconds
        out = []
        while time.monotonic() < end:
            snapshot = sample(label)
            out.append(snapshot)
            if snapshot["player"]["hp"] < 200:
                break
            time.sleep(0.04)
        return out

    def move_to_fixture(xyz, yaw=0):
        command(pid, mode=2)
        time.sleep(0.6)
        host.warp(xyz, yaw)
        assert host.focused()
        driver.cmd("hold 0x4F 140", retry=False)
        time.sleep(0.7)
        command(pid, mode=1)
        observe("activate", 0.8)

    try:
        start = sample("start")
        assert start["equipment"]["stance"] == 1
        if stage == "safe":
            move_to_fixture([3.1, 196.5, 5.7], math.pi)
            command(pid, frames=90, buttons=8)
            guard = observe("guard", 1.4)
            shot(str(output / "shield-guard.png"), exe="DARKSOULS.exe", scale=0.5)
            command(pid)
            released = observe("release", 0.7)
            command(pid, frames=8, buttons=9)
            jump = observe("guard_jump", 2.5)
            assert any(snapshot["native_guard"] for snapshot in guard), (
                "Native guard never became active"
            )
            assert not released[-1]["native_guard"], "Guard survived release"
            assert all(snapshot["equipment"]["stance"] == 1 for snapshot in rows), (
                "Guard changed handedness"
            )
            assert (
                max(snapshot["player"]["position"][1] for snapshot in jump)
                - released[-1]["player"]["position"][1]
                > 2
            )
            assert (
                min(snapshot["player"]["stamina"] for snapshot in jump)
                >= released[-1]["player"]["stamina"]
            )
        elif stage in ("front", "rear"):
            target = start["target"]
            assert target and target["hp"] > 0
            x, y, z = target["position"]
            move_to_fixture([x, y + 0.2, z + 1.3], 0 if stage == "front" else math.pi)
            command(pid, frames=150, buttons=8)
            observe(stage, 4.8)
            shot(str(output / (stage + "-block.png")), exe="DARKSOULS.exe", scale=0.5)
        else:
            raise ValueError("Unknown stage")
    finally:
        try:
            command(pid)
            if stage in ("front", "rear"):
                command(pid, mode=2)
                time.sleep(0.6)
                host.warp([3.1, 196.5, 5.7], math.pi)
                time.sleep(0.7)
                command(pid, mode=1)
        finally:
            (output / (stage + "-trace.json")).write_text(json.dumps(rows, indent=2))
            log = (GAME / "mariosouls.log").read_bytes()[logstart:]
            (output / (stage + "-log.txt")).write_bytes(log)
            driver.close()
            host.close()
    print(
        json.dumps(
            dict(
                stage=stage,
                samples=len(rows),
                hp=[rows[0]["player"]["hp"], rows[-1]["player"]["hp"]],
                stamina_min=min(snapshot["player"]["stamina"] for snapshot in rows),
                guard_samples=sum(snapshot["native_guard"] for snapshot in rows),
                passive_samples=sum(not snapshot["native_active"] for snapshot in rows),
            )
        )
    )


if __name__ == "__main__":
    main()
