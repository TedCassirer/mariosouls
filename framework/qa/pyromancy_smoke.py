"""Record real native fireball casts on Mario QA. No spell/HP/projectile writes.

Requires an equipped flame and Fireball already attuned through native menus.
Rest between combat runs. The fixed Asylum fixture is the same guarded QA hollow
used for criticals; never infer success merely from sending the attack button.
"""

from pathlib import Path
import argparse
import json
import time
import math


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("pid", type=int)
    p.add_argument("--hand", choices=["right", "left"], default="right")
    p.add_argument("--output", required=True)
    p.add_argument("--enemy", action="store_true")
    p.add_argument("--record", action="store_true")
    p.add_argument("--target", type=int, choices=[1810101, 1810103], default=1810101)
    p.add_argument(
        "--native",
        action="store_true",
        help="Compare the unmodified host cast with character control disabled",
    )
    p.add_argument("--distance", type=float, default=3.0)
    p.add_argument(
        "--yaw",
        type=float,
        default=0,
        help="Courtyard throw heading, radians; enemy fixture overrides this",
    )
    a = p.parse_args()

    from host import Host, GAME
    from bridge import command
    from um.win import Recorder, shot, Drive

    out = Path(a.output)
    out.mkdir(parents=True, exist_ok=False)
    h = Host(a.pid, write=True)
    rows = []
    rec = None
    logstart = (GAME / "mariosouls.log").stat().st_size

    def send(**kw):
        command(a.pid, host=h, **kw)

    try:
        s = h.snapshot(a.target)
        assert s["focus"] and not s["menu"] and s["player"]["hp"] >= 500
        assert s["equipment"]["right1" if a.hand == "right" else "left1"] == 1330000
        if a.enemy:
            assert s["target"] and s["target"]["hp"] > 0
        if a.record:
            rec = Recorder(exe="DARKSOULS.exe", out=str(out / "take"), fps=30, audio=True)
            rec.start()
        send(mode=2)
        time.sleep(0.6)
        if a.enemy:
            assert 1 <= a.distance <= 4
            x, y, z = s["target"]["position"]
            h.warp(
                [x, y + 0.05, z + (-a.distance if a.target == 1810103 else a.distance)],
                math.pi if a.target == 1810103 else 0,
                a.target,
            )
        else:
            assert math.isfinite(a.yaw)
            h.warp([3.1, 196.1, 5.7], a.yaw)
        time.sleep(0.3)
        if not a.native:
            send(mode=1)
        time.sleep(0.7)
        if a.enemy:
            d = Drive("DARKSOULS")
            try:
                d.hold("0x4F", 140)
                time.sleep(0.2)  # Native camera reset / lock-on.
            finally:
                d.close()
        if not a.enemy:
            shot(str(out / "before.png"), exe="DARKSOULS.exe", scale=0.5)
        if a.native:
            d = Drive("DARKSOULS")
            try:
                d.hold("0x48" if a.hand == "right" else "0xA0", 140)
            finally:
                d.close()
        else:
            send(frames=5, buttons=2 if a.hand == "right" else 16)
        start = time.monotonic()
        while time.monotonic() - start < 5:
            s = h.snapshot(a.target)
            s["animation_rate"] = h.f(
                h.chain(int(s["player"]["reference"], 16) + 0x28, 0, 0x14) + 0x64
            )
            rows.append(s)
            if not s["focus"] or s["menu"] or s["player"]["hp"] < 500:
                break
            time.sleep(0.04)
        shot(str(out / "after.png"), exe="DARKSOULS.exe", scale=0.5)
    finally:
        if rec:
            rec.stop()
        (out / "trace.json").write_text(json.dumps(rows, indent=2), encoding="utf-8")
        (out / "log.txt").write_bytes((GAME / "mariosouls.log").read_bytes()[logstart:])
        try:
            s = h.snapshot()
            if s["focus"] and not s["menu"] and s["player"]["hp"] > 0:
                send(mode=2)
                time.sleep(0.5)
                h.warp([3.1, 196.5, 5.7], math.pi)
                time.sleep(0.5)
                send(mode=1)
        finally:
            h.close()
    if rows:
        print(
            json.dumps(
                dict(
                    hp=[rows[0]["player"]["hp"], rows[-1]["player"]["hp"]],
                    target_hp=[
                        rows[0]["target"]["hp"] if rows[0]["target"] else None,
                        rows[-1]["target"]["hp"] if rows[-1]["target"] else None,
                    ],
                    tracks=[
                        (s["tick"], s["player"]["tracks"])
                        for i, s in enumerate(rows)
                        if i == 0 or s["player"]["tracks"] != rows[i - 1]["player"]["tracks"]
                    ],
                )
            )
        )
    print(
        "Evidence only: verify projectile hit, native charges and return from the trace and recording."
    )


if __name__ == "__main__":
    main()
