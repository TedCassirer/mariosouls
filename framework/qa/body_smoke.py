"""Focused body/fall regression. Inputs and warps require living Mario QA."""

from pathlib import Path
import time
import json
import math
import argparse


def main(arguments=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pid", type=int)
    parser.add_argument("stage", choices=["cycles", "rest", "enemy", "door", "incoming", "drop"])
    parser.add_argument("height", nargs="?", type=int, help="Drop height for the drop stage")
    parser.add_argument(
        "--output", type=Path, default=Path(__file__).resolve().parents[2] / "work/body-qa"
    )
    args = parser.parse_args(arguments)
    if args.stage == "drop" and args.height is None:
        parser.error("the drop stage requires a height")
    if args.stage != "drop" and args.height is not None:
        parser.error("height only applies to the drop stage")

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

    def sample(label, target=1810101):
        snapshot = host.snapshot(target)
        assert snapshot["focus"] and snapshot["player"]["hp"] > 0
        assert not any(snapshot["protections"].values()), snapshot["protections"]
        physics = host.chain(int(snapshot["player"]["reference"], 16), 0x28, 0x1C)
        shapes = [host.u(physics + 0x3C), host.u(physics + 0x40), host.chain(physics, 0x48, 0x1C)]
        snapshot["body"] = [
            dict(radius=host.f(a + 0x10), top=host.f(a + 0x24), bottom=host.f(a + 0x34))
            for a in shapes
        ]
        snapshot["body_height"] = host.f(physics + 0x204)
        snapshot["body_radius"] = host.f(physics + 0x208)
        snapshot["label"] = label
        rows.append(snapshot)
        return snapshot

    def observe(label, seconds, target=1810101):
        end = time.monotonic() + seconds
        out = []
        while time.monotonic() < end:
            out.append(sample(label, target))
            time.sleep(0.04)
        return out

    def key(vk, ms=140):
        assert host.focused()
        driver.cmd("hold " + vk + " " + str(ms), retry=False)

    def move_to_fixture(xyz, yaw=0):
        command(pid, mode=2)
        observe("native_before_warp", 0.6)
        host.warp(xyz, yaw)
        key("0x4F")
        observe("settle", 0.7)
        command(pid, mode=1)
        observe("activated", 0.8)

    try:
        sample("start")
        if stage == "cycles":
            for n in range(3):
                command(pid, mode=2)
                snapshot = observe("native_" + str(n), 0.6)[-1]
                assert (
                    abs(snapshot["body"][0]["radius"] - 0.4) < 0.001
                    and abs(snapshot["body"][2]["radius"] - 0.45) < 0.001
                )
                assert abs(snapshot["body_height"] - 1.5) < 0.001
                command(pid, mode=1)
                snapshot = observe("mario_" + str(n), 0.6)[-1]
                assert (
                    abs(snapshot["body"][0]["radius"] - 0.22) < 0.001
                    and abs(snapshot["body"][2]["radius"] - 0.24) < 0.001
                )
                assert abs(snapshot["body_height"] - 1.05) < 0.001
            y = snapshot["player"]["position"][1]
            sp = snapshot["player"]["stamina"]
            command(pid, frames=8, buttons=1)
            jump = observe("jump", 2.8)
            assert 2.3 < max(snapshot["player"]["position"][1] for snapshot in jump) - y < 2.6
            assert min(snapshot["player"]["stamina"] for snapshot in jump) >= sp
        elif stage == "rest":
            move_to_fixture([3.1, 196.5, 6.55], math.pi)
            command(pid, mode=4)
            rest = observe("rest", 3)
            assert any(snapshot["menu"] for snapshot in rest)
            key("0x0D")
            leave = observe("leave", 3)
            assert not leave[-1]["menu"] and not leave[-1]["interaction"]
            assert all(
                abs(snapshot["body"][2]["radius"] - 0.24) < 0.001 for snapshot in rest + leave
            )
            command(pid, frames=8, buttons=1)
            observe("post_rest_jump", 2.5)
        elif stage == "enemy":
            move_to_fixture([-14.668, 185.8, -21.4])
            command(pid, frames=75, y=1)
            observe("enemy_push", 3.3)
            command(pid)
            observe("enemy_release", 0.6)
        elif stage == "door":
            move_to_fixture([3.2, 198.2, -1.8])
            command(pid, frames=75, y=1)
            observe("door_push", 3.3)
            command(pid)
            observe("door_release", 0.6)
        elif stage == "incoming":
            try:
                move_to_fixture([-7.76, 208.6, -3.8])
                end = time.monotonic() + 5
                while time.monotonic() < end:
                    snapshot = sample("enemy_melee", 1810103)
                    if snapshot["player"]["hp"] < 200:
                        break
                    time.sleep(0.04)
            finally:
                # Do not leave a living QA character unattended beside an attacking enemy.
                command(pid, mode=2)
                observe("retreat_release", 0.6)
                host.warp([3.1, 196.5, 6.55], math.pi)
                observe("retreated", 0.6)
        elif stage == "drop":
            # Existing guarded transport owns heartbeat/target identity; does not change HP.
            from transport import Session

            move_to_fixture([3.1, 196.5, 5.7], math.pi)
            height = args.height
            session = Session(1810960, suppress_outgoing=True, pid=pid)
            try:
                session.command(3, 20)
                session.command(5, frames=height)
                end = time.monotonic() + 7
                while time.monotonic() < end:
                    session.poll()
                    sample("drop", 1810960)
                    time.sleep(0.04)
            finally:
                events = session.close()
                (output / ("drop-" + str(height) + "-events.json")).write_text(
                    json.dumps(events, indent=2)
                )
        else:
            raise ValueError(stage)
        shot(str(output / (stage + ".png")), exe="DARKSOULS.exe", scale=0.5)
        print(
            json.dumps(
                {
                    "stage": stage,
                    "first_hp": rows[0]["player"]["hp"],
                    "last_hp": rows[-1]["player"]["hp"],
                    "last_position": rows[-1]["player"]["position"],
                    "last_body": rows[-1]["body"],
                }
            )
        )
    finally:
        (
            output / (stage + ("-" + str(args.height) if args.height is not None else "") + ".json")
        ).write_text(json.dumps(rows, indent=2))
        (
            output / (stage + ("-" + str(args.height) if args.height is not None else "") + ".log")
        ).write_bytes((GAME / "mariosouls.log").read_bytes()[logstart:])
        try:
            command(pid)
        finally:
            host.close()
            driver.close()


if __name__ == "__main__":
    main()
