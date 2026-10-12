"""Run a named live-game QA scenario. Listing scenarios never opens a game."""

import argparse
from pathlib import Path
import subprocess
import sys

SCENARIOS = {
    "smoke": "smoke.py",
    "body": "body_smoke.py",
    "contact": "contact_smoke.py",
    "camera": "camera_smoke.py",
    "equipment": "equipment_smoke.py",
    "critical": "critical_smoke.py",
    "kick": "kick_smoke.py",
    "pyromancy": "pyromancy_smoke.py",
    "launcher": "launcher_smoke.py",
    "body-probe": "body_probe.py",
    "contact-probe": "contact_probe.py",
    "starting-class": "starting_class.py",
}


def main(arguments=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scenario", nargs="?", choices=SCENARIOS)
    parser.add_argument("arguments", nargs=argparse.REMAINDER, help="Arguments for the scenario")
    args = parser.parse_args(arguments)
    if args.scenario is None:
        parser.print_help()
        return 0
    script = Path(__file__).resolve().parent / SCENARIOS[args.scenario]
    return subprocess.run([sys.executable, str(script), *args.arguments]).returncode


if __name__ == "__main__":
    sys.exit(main())
