"""Compare native/character camera distances on focused Mario QA; read-only observation plus mode switches."""

from pathlib import Path
import argparse
import json
import math
import time


def observe(h):
    s = h.snapshot()
    assert s["focus"] and not s["menu"] and s["player"]["hp"] > 0
    cam = h.chain(h.protection_slots["camera"], 0, 0x3C, 0x60)
    s["camera_eye"] = h.v(cam + 0x40)
    s["camera_forward"] = h.v(cam + 0x30)
    target = list(s["player"]["position"])
    target[1] += 1.42
    s["camera_target_distance"] = math.dist(s["camera_eye"], target)
    repo = h.u(0x13785B4)
    p = h.chain(repo + 31 * 36 + 12, 0, 0x20)
    assert h.read(p + 12, 18) == b"LOCK_CAM_PARAM_ST\0"
    n = int.from_bytes(h.read(p + 10, 2), "little")
    assert 0 < n <= 512
    s["camera_rows"] = [
        dict(id=h.i(p + 48 + i * 12), distance=h.f(p + h.u(p + 52 + i * 12))) for i in range(n)
    ]
    return s


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("pid", type=int)
    p.add_argument("--output", required=True)
    a = p.parse_args()

    from host import Host
    from bridge import command
    from um.win import Drive, shot

    out = Path(a.output)
    out.mkdir(parents=True, exist_ok=False)
    h = Host(a.pid)
    d = None
    states = {}
    try:
        observe(h)
        command(a.pid, mode=2, host=h)
        time.sleep(1)
        d = Drive("DARKSOULS")
        d.hold("0x4F", 140)
        time.sleep(2)
        for label, mode in [("native", 2), ("character", 1), ("restored", 2)]:
            command(a.pid, mode=mode, host=h)
            time.sleep(3)
            states[label] = observe(h)
            shot(str(out / (label + ".png")), exe="DARKSOULS.exe", scale=0.5)
        assert states["native"]["camera_rows"] == states["restored"]["camera_rows"], (
            "Native rows not restored"
        )
        for before, after in zip(
            states["native"]["camera_rows"], states["character"]["camera_rows"]
        ):
            assert (
                before["id"] == after["id"]
                and abs(after["distance"] - before["distance"] * 1.5) < 1e-5
            )
        print({key: round(value["camera_target_distance"], 3) for key, value in states.items()})
        print(
            "All native camera rows scaled once and restored exactly. Inspect images for framing/collision."
        )
    finally:
        (out / "trace.json").write_text(json.dumps(states, indent=2))
        if d:
            d.close()
        try:
            if h.snapshot()["focus"]:
                command(a.pid, mode=1, host=h)
        finally:
            h.close()


if __name__ == "__main__":
    main()
