"""Short, guarded gameplay regression on a safe, flat Mario QA location.

Run after visually checking the character is clear of ledges and enemies.
No save, health, inventory or AI writes; inputs use the bounded host bridge.
"""

from pathlib import Path
import argparse
import json
import time
from host import Host
from bridge import command


def main(pid):
    h = Host(pid)
    evidence = []

    def observe(label, seconds):
        rows = []
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            s = h.snapshot()
            if not s["focus"] or s["player"]["hp"] <= 0 or s["interaction"] or s["menu"]:
                raise RuntimeError("Focus, life or native action changed; stopped")
            rows.append(
                {
                    "tick": s["tick"],
                    "position": s["player"]["position"],
                    "hp": s["player"]["hp"],
                    "stamina": s["player"]["stamina"],
                    "protections": s["protections"],
                }
            )
            time.sleep(0.03)
        evidence.append({"check": label, "samples": rows})
        return rows

    try:
        baseline = observe("baseline", 0.4)[-1]
        command(pid, frames=8, buttons=1)
        jump = observe("jump", 3)
        height = max(r["position"][1] for r in jump) - baseline["position"][1]
        assert height > 0.5, "Jump did not move native body"
        assert min(r["stamina"] for r in jump) == baseline["stamina"], (
            "Navigation jump consumed stamina"
        )
        command(pid, frames=2, buttons=2)
        attack = observe("first_punch", 2)
        spent = baseline["stamina"] - min(r["stamina"] for r in attack)
        assert spent > 0, "Attack did not consume stamina"
        assert all(r["hp"] == baseline["hp"] for e in evidence for r in e["samples"]), (
            "Unexpected HP change in safe scene"
        )
        out = Path(__file__).resolve().parents[2] / "work/framework-build/live-smoke.json"
        out.write_text(
            json.dumps(
                {
                    "character": "Mario QA",
                    "jump_height_m": height,
                    "jump_stamina_cost": 0,
                    "attack_stamina_cost": spent,
                    "evidence": evidence,
                },
                indent=2,
            )
        )
        print(
            json.dumps(
                {
                    "jump_height_m": height,
                    "jump_stamina_cost": 0,
                    "first_punch_cost": spent,
                    "report": str(out),
                }
            )
        )
    finally:
        try:
            command(pid)
        finally:
            h.close()


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("pid", type=int)
    main(p.parse_args().pid)
