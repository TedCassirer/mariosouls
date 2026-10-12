"""Record kick/poise evidence on Mario QA using real movement and native enemies.

No enemy position, HP, AI or poise writes. A low configured FixedPower is useful
for observing living stagger reactions; restore the setting after the test pass.
"""

from pathlib import Path
import argparse
import json
import time
import math


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("pid", type=int)
    p.add_argument("move", choices=["combo", "jump_kick", "slide_kick", "punch"])
    p.add_argument("--target", type=int, choices=[1810101, 1810103, 1810800], default=1810103)
    p.add_argument("--distance", type=float, default=0.9)
    p.add_argument("--output", required=True)
    p.add_argument("--record", action="store_true")
    a = p.parse_args()

    from host import Host, GAME
    from bridge import command
    from um.win import Drive, Recorder, shot

    assert 0.5 <= a.distance <= 5
    out = Path(a.output)
    out.mkdir(parents=True, exist_ok=False)
    h = Host(a.pid, write=True)
    d = Drive("DARKSOULS")
    rows = []
    rec = None
    startlog = (GAME / "mariosouls.log").stat().st_size

    def send(**kw):
        command(a.pid, host=h, **kw)

    def observe(seconds):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            s = h.snapshot(a.target)
            rows.append(s)
            if not s["focus"] or s["menu"] or s["player"]["hp"] < 350:
                raise RuntimeError("Focus/menu/health guard")
            time.sleep(0.025)

    try:
        s = h.snapshot(a.target)
        assert s["focus"] and not s["menu"] and s["player"]["hp"] >= 500
        assert s["held_right_id"] == 900000, "Unequip right hand before guest kick checks"
        assert s["target"] and s["target"]["hp"] > 0
        if a.target == 1810103:
            assert s["target"]["position"][1] > 208, "Rest first: hollow left its fixture"
        x, y, z = s["target"]["position"]
        yaw = s["target"]["facing"]
        assert a.target != 1810800 or y < 200, "Boss must already be on arena floor"
        # Approach from behind the existing enemy, without changing its state.
        send(mode=2)
        time.sleep(0.6)
        h.warp(
            [x + math.sin(yaw) * a.distance, y + 0.05, z + math.cos(yaw) * a.distance],
            yaw,
            a.target,
        )
        d.hold("0x4F", 140)
        time.sleep(0.4)
        send(mode=1)
        observe(0.7)
        if a.record:
            rec = Recorder(exe="DARKSOULS.exe", out=str(out / "take"), fps=30, audio=True)
            rec.start()
        if a.move == "combo":
            for _ in range(3):
                send(frames=3, buttons=2)
                observe(0.27)
        elif a.move == "jump_kick":
            send(frames=8, buttons=1)
            observe(0.3)
            send(frames=6, buttons=2)
        elif a.move == "slide_kick":
            send(frames=12, y=1)
            observe(0.45)
            send(frames=4, buttons=4, y=1)
            observe(0.15)
            send(frames=6, buttons=6, y=1)
        else:
            send(frames=4, buttons=2)
        observe(2.4)
        shot(str(out / "after.png"), exe="DARKSOULS.exe", scale=0.5)
    finally:
        if rec:
            rec.stop()
        (out / "trace.json").write_text(json.dumps(rows, indent=2))
        (out / "log.txt").write_bytes((GAME / "mariosouls.log").read_bytes()[startlog:])
        try:
            s = h.snapshot()
            if s["focus"] and not s["menu"] and s["player"]["hp"] > 0:
                send(mode=2)
                time.sleep(0.6)
                h.warp([3.1, 196.5, 5.7], math.pi)
                time.sleep(0.6)
                send(mode=1)
        finally:
            d.close()
            h.close()
    print(
        "Recorded native HP, poise, positions and animations. Inspect actual contact and displacement before claiming a pass."
    )


if __name__ == "__main__":
    main()
