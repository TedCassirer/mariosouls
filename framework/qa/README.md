# Live-game QA

These are specialized regression scenarios, not the automated CI suite. They
observe or control a running Dark Souls process and may change the disposable
test character's position or active mode. Use the supported offline suite first:

```powershell
python tools/framework.py test --shared-only
```

Live scenarios require the installed mod, Windows, the optional Universal Modder
Python tooling, backups and a current handoff of the controls. List scenarios
through the same entry point used for other development commands:

```powershell
python tools/framework.py qa
python tools/framework.py qa contact --help
python tools/framework.py qa body --help
```

Each scenario's `--help` describes its arguments and runs without optional game
dependencies. Listing and help never open the game or acquire controls. For
example, after a control handoff with a prepared Mario QA fixture:

```powershell
python tools/framework.py qa contact 12345 door --output work/qa/door-run
```

Replace the example PID with the running game process ID. Never use these against a player's ordinary
save or bypass a character/level guard to make a test run.

## Shared infrastructure

`host.py` owns native observations and validates character identity for guarded
operations. `bridge.py` sends bounded commands through the host's opt-in test
interface. `transport.py` manages command leases and telemetry for the frame
harness. These helpers remain in the host QA layer; character adapters do not
gain native memory access through them.

## Scenarios

| Script | Purpose |
| --- | --- |
| `smoke.py` | Short gameplay regression in a safe, flat location |
| `body_smoke.py` | Body dimensions and fall behavior |
| `contact_smoke.py` | Doorway and boss contact regressions |
| `camera_smoke.py` | Native and character camera comparison |
| `equipment_smoke.py` | Equipment state and rendered behavior |
| `critical_smoke.py` | Observed native parry and critical sequences |
| `kick_smoke.py` | Kick impact and enemy poise evidence |
| `pyromancy_smoke.py` | Native fireball casting evidence |
| `launcher_smoke.py` | Firelink cannon aim and launch |
| `body_probe.py` / `contact_probe.py` | Read-only body and contact measurements |
| `starting_class.py` | Read-only starting stats for Mario Class QA |
| `render_equipment.py` | Offline rendering of private SDK pose dumps |

Most scenarios require Mario QA; the body probe and starting-class probe have
their own narrow fixture identities. The guards in each scenario are the source
of truth. No scenario should target the owner's level-65 character.

The renderer is part of the supported `preview-equipment` workflow, not a live
scenario. Exit the game through its native menu.

Write captures and measurements to ignored `work/` or `outputs/`. Do not commit
saves, raw process observations or game-derived images. Record concise findings
in [Validation](../docs/VALIDATION.md), distinguishing live observations from
offline test results. See [Testing](../../docs/TESTING.md) for the remaining
release matrix.
