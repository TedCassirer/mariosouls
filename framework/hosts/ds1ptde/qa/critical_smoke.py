"""Record native parry/critical evidence on Mario QA, never force a successful hit.

Native enemy AI, HP, stamina and throw eligibility remain untouched. The only
fixture write is the existing bounded Asylum warp. Rest at the bonfire between
runs. --timing adjusts test input timing, not the mod's parry window.
"""

from pathlib import Path
import argparse
import json
import math
import time


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("pid", type=int)
    p.add_argument("stage", choices=["backstab", "parry", "front"])
    p.add_argument("--timing", type=float, default=0.5)
    p.add_argument("--output", required=True)
    p.add_argument("--record", action="store_true")
    args = p.parse_args()

    from host import Host, GAME
    from bridge import command
    from um.win import Drive, Recorder

    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=False)
    h = Host(args.pid, write=True)
    d = Drive("DARKSOULS")
    rows = []
    rec = None

    def send(**kw):
        command(args.pid, host=h, **kw)

    logstart = (GAME / "mariosouls.log").stat().st_size

    def sample():
        s = h.snapshot(1810103)
        if not s["focus"] or s["menu"] or s["player"]["hp"] <= 0:
            raise RuntimeError("Game focus, menu or life changed")
        if s["target"]:
            t = h.chain(int(s["target"]["reference"], 16), 0x28, 0x18)
            s["enemy_times"] = [(j, a, h.f(t + j * 0x9C + 0x98)) for j, a in s["target"]["tracks"]]
        else:
            s["enemy_times"] = []
        rows.append(s)
        return s

    try:
        s = sample()
        deadline = time.monotonic() + 5
        while not s["target"] and time.monotonic() < deadline:
            time.sleep(0.1)
            s = sample()
        assert s["target"] and s["target"]["hp"] > 0 and s["player"]["hp"] >= 500
        assert s["equipment"]["stance"] == 1
        if args.record:
            rec = Recorder(exe="DARKSOULS.exe", out=str(out / "take"), fps=30, audio=True)
            rec.start()
        x, y, z = s["target"]["position"]
        send(mode=2)
        time.sleep(0.5)
        rear = args.stage == "backstab"
        h.warp([x, y + 0.05, z + (-0.85 if rear else 1.05)], math.pi if rear else 0, 1810103)
        send(mode=1)
        time.sleep(0.5)  # Allow the polled command to activate before another command.
        fired = False
        attack = False
        approached = False
        firetime = 0
        start = time.monotonic()
        start_hp = s["player"]["hp"]
        if args.stage != "parry":
            send(frames=5, buttons=2 if rear else 10)
        while time.monotonic() - start < 10:
            s = sample()
            if args.stage == "parry" and not fired:
                # The hollow's windups differ. Test its actual native animation clock.
                for _, a, sec in s["enemy_times"]:
                    offset = {3000: 0, 3001: -0.2, 3002: 0, 3003: 0.45, 3008: 0}.get(a % 10000)
                    if offset is not None and sec >= max(0, args.timing + offset):
                        d.cmd("hold 0x09 80", retry=False)
                        fired = True
                        firetime = time.monotonic()
                        break
            if (
                fired
                and not attack
                and time.monotonic() - firetime > 1.25
                and s["player"]["hp"] == start_hp
            ):
                distance = (
                    math.dist(s["player"]["position"], s["target"]["position"])
                    if s["target"]
                    else 99
                )
                if not approached:
                    c = h.camera_forward()
                    a = s["player"]["position"]
                    b = s["target"]["position"]
                    dx = b[0] - a[0]
                    dz = b[2] - a[2]
                    forward = dx * c[0] + dz * c[2]
                    right = dx * c[2] - dz * c[0]
                    send(
                        frames=18,
                        x=(1 if right > 0 else -1) if abs(right) > abs(forward) else 0,
                        y=(1 if forward > 0 else -1) if abs(forward) >= abs(right) else 0,
                    )
                    approached = True
                    firetime = time.monotonic() - 1
                elif distance < 0.85 or time.monotonic() - firetime > 2.1:
                    send(frames=5, buttons=2)
                    attack = True
            if s["player"]["hp"] < 500:
                break
            time.sleep(0.025)
    finally:
        # Do not tear down/warp a paired throw while the native sequence is running.
        deadline = time.monotonic() + 8
        while time.monotonic() < deadline:
            s = h.snapshot(1810103)
            if (
                not s["focus"]
                or s["player"]["hp"] <= 0
                or not any(j == 28 and a >= 0 for j, a in s["player"]["tracks"])
            ):
                break
            time.sleep(0.05)
        if rec:
            rec.stop()
        (out / "trace.json").write_text(json.dumps(rows, indent=2), encoding="utf-8")
        (out / "log.txt").write_bytes((GAME / "mariosouls.log").read_bytes()[logstart:])
        try:
            s = h.snapshot()
            if s["focus"] and s["player"]["hp"] > 0 and not s["menu"]:
                send(mode=2)
                time.sleep(0.5)
                h.warp([3.1, 196.5, 5.7], math.pi)
                time.sleep(0.6)
                send(mode=1)
        finally:
            d.close()
            h.close()
    changes = [
        dict(
            tick=s["tick"],
            player=s["player"]["tracks"],
            enemy=s["enemy_times"],
            hp=s["player"]["hp"],
            target_hp=s["target"]["hp"] if s["target"] else None,
        )
        for i, s in enumerate(rows)
        if i == 0
        or s["player"]["tracks"] != rows[i - 1]["player"]["tracks"]
        or s["target"] != rows[i - 1]["target"]
        and (
            not s["target"]
            or not rows[i - 1]["target"]
            or s["target"]["tracks"] != rows[i - 1]["target"]["tracks"]
            or s["target"]["hp"] != rows[i - 1]["target"]["hp"]
        )
    ]
    print(json.dumps(changes))
    print("Evidence recorded; inspect native tracks and HP before declaring success.")


if __name__ == "__main__":
    main()
