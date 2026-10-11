"""Repeat doorway/boss regressions on living, focused Mario QA only.

Boss stage requires the existing boss on the arena floor. No spawning, health,
inventory, quest, or enemy-memory writes. Always retreat to the bonfire.
"""

from pathlib import Path
import argparse
import json
import math
import time


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("pid", type=int)
    p.add_argument("stage", choices=["door", "boss"])
    p.add_argument("--output", type=Path, required=True)
    a = p.parse_args()

    from host import Host, GAME
    from bridge import command
    from contact_probe import observe
    from um.win import Drive, shot

    a.output.mkdir(parents=True, exist_ok=True)
    h = Host(a.pid, write=True)
    d = Drive("DARKSOULS")
    rows = []
    start = (GAME / "mariosouls.log").stat().st_size

    def record(seconds):
        until = time.monotonic() + seconds
        while time.monotonic() < until:
            s = observe(h)
            rows.append(s)
            if not s["focus"] or s["player"]["hp"] < 200:
                raise RuntimeError("Focus/health guard: stop and retreat")
            time.sleep(0.04)

    try:
        s = h.snapshot(1810800)
        assert s["focus"] and not s["menu"] and not s["interaction"] and s["player"]["hp"] > 300
        command(h.pid, mode=2)
        time.sleep(0.4)
        if a.stage == "door":
            pos = [14, 198.3, -25.4]
            yaw = -math.pi / 2
        else:
            boss = s["target"]
            assert boss and boss["hp"] > 0 and boss["position"][1] < 200, (
                "Existing boss must be active on the arena floor"
            )
            x, y, z = boss["position"]
            pos = [x, y + 0.3, z + 3.2]
            yaw = 0
        h.warp(pos, yaw)
        d.cmd("hold 0x4F 120", retry=False)
        time.sleep(0.4)
        command(h.pid, mode=1)
        record(0.4)
        before = rows[-1]
        if a.stage == "door":
            command(h.pid, frames=90, y=1)
            record(3.4)
            command(h.pid)
            assert max(s["player"]["position"][0] for s in rows) > 17.5, (
                "Did not cross the actual threshold"
            )
        else:
            for delay in (0.22, 0.5, 0.65):
                d.cmd("mdown 960 540", retry=False)
                time.sleep(0.08)
                d.cmd("mup", retry=False)
                record(delay)
            assert rows[-1]["target"]["hp"] < before["target"]["hp"], "No native boss HP loss"
        shot(str((a.output / (a.stage + ".png")).resolve()), exe="DARKSOULS.exe", scale=0.5)
        print(
            json.dumps(
                dict(
                    stage=a.stage,
                    result="passed",
                    before=before["player"]["position"],
                    after=rows[-1]["player"]["position"],
                    boss_hp_before=before["target"]["hp"] if before["target"] else None,
                    boss_hp_after=rows[-1]["target"]["hp"] if rows[-1]["target"] else None,
                )
            )
        )
    finally:
        try:
            # The bridge refuses identity/focus/death changes. Never refocus.
            command(h.pid, mode=2)
            time.sleep(0.4)
            h.warp([3.1, 196.5, 6.55], math.pi)
        finally:
            h.close()
            d.close()
            (a.output / (a.stage + ".json")).write_text(json.dumps(rows, indent=2))
            (a.output / (a.stage + ".log")).write_bytes(
                (GAME / "mariosouls.log").read_bytes()[start:]
            )


if __name__ == "__main__":
    main()
