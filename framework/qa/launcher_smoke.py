"""Bounded, foreground-only Firelink launcher checks on Mario QA.

No native memory writes. Navigation uses ordinary keyboard input; identity is
checked before every pulse. Capture trace/screenshot and inspect the result.
"""

from pathlib import Path
import argparse
import json
import math
import time
from host import Host
from um.win import Drive, shot, Recorder


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("pid", type=int)
    p.add_argument("--walk", nargs=2, type=float, metavar=("X", "Z"))
    p.add_argument("--tap", choices=["interact", "toggle", "jump", "camera"])
    p.add_argument("--aim-up-ms", type=int, default=0)
    p.add_argument("--fire", action="store_true")
    p.add_argument("--seconds", type=float, default=2)
    p.add_argument("--output", required=True)
    p.add_argument("--record", action="store_true")
    a = p.parse_args()
    assert 0 <= a.seconds <= 15 and 0 <= a.aim_up_ms <= 1000
    if a.walk:
        assert all(math.isfinite(x) for x in a.walk) and math.dist(a.walk, [-45, 56]) < 10
    out = Path(a.output)
    out.mkdir(parents=True, exist_ok=False)
    h = Host(a.pid, maps=(1002,))
    d = Drive("DARKSOULS")
    rec = None
    rows = []

    def observe():
        s = h.snapshot()
        assert s["focus"] and s["player"]["hp"] > 0 and not s["menu"] and not s["interaction"], (
            "QA identity/focus/health/menu boundary"
        )
        rows.append(s)
        return s

    try:
        observe()
        if a.record:
            rec = Recorder(exe="DARKSOULS.exe", out=str(out / "take"), fps=30, audio=True)
            rec.start()
        if a.walk:
            for _ in range(100):
                s = observe()
                x, _, z = s["player"]["position"]
                dx, dz = a.walk[0] - x, a.walk[1] - z
                if math.hypot(dx, dz) < 0.4:
                    break
                assert math.hypot(dx, dz) < 12, "Navigation left the local fixture"
                cx, _, cz = h.camera_forward()
                norm = math.hypot(cx, cz)
                assert norm > 0.1
                cx /= norm
                cz /= norm
                right, forward = dx * cz - dz * cx, dx * cx + dz * cz
                # Short axis pulses avoid holding input across observation or focus changes.
                key = (
                    ("0x44" if right > 0 else "0x41")
                    if abs(right) > abs(forward)
                    else ("0x57" if forward > 0 else "0x53")
                )
                d.hold(key, 100)
                time.sleep(0.04)
            else:
                raise RuntimeError("Navigation deadline; inspect obstacle, never warp through it")
        if a.tap:
            observe()
            d.hold(dict(interact="0x51", toggle="0x75", jump="0x20", camera="0x4F")[a.tap], 140)
        if a.aim_up_ms:
            observe()
            d.hold("0x57", a.aim_up_ms)
        if a.fire:
            observe()
            d.hold("0x20", 140)
        until = time.monotonic() + a.seconds
        while time.monotonic() < until:
            observe()
            time.sleep(0.08)
        shot(str(out / "after.png"), exe="DARKSOULS.exe", scale=0.5)
    finally:
        if rec:
            rec.stop()
        (out / "trace.json").write_text(json.dumps(rows, indent=2))
        d.close()
        h.close()
    if rows:
        print(
            json.dumps(
                dict(
                    name=rows[-1]["player"]["name"],
                    position=rows[-1]["player"]["position"],
                    hp=rows[-1]["player"]["hp"],
                )
            )
        )


if __name__ == "__main__":
    main()
