# Validation and coverage

MarioSouls is an experimental development build. Offline checks pass, and earlier
builds have targeted live measurements in the Asylum and Firelink. Neither is
evidence of full-game compatibility or a release-ready installation.

## Automated checks

The latest repository-cleanup pass rebuilt the host and Mario adapter and passed
the full offline suite with the real Mario runtime and an owned US ROM. It did
not install the refactor or drive the game.

| Layer | Evidence | Limits |
| --- | --- | --- |
| Shared policies | Lifecycle, motion, input, combat, body, contact and feedback tests | Synthetic state; no running game |
| Native contracts | Hook thunks, target identity, blocking and guard-break fixtures | Fake callees and packets do not prove native combat in every encounter |
| Rendering | Local D3D9 composition, depth and UI ordering tests | Hosted CI skips rendering; not a display/driver compatibility matrix |
| Mario runtime | Real adapter mesh, jump, traction, missing-floor, death, equipment and lifecycle tests | Synthetic surfaces and pose fixtures do not replace level traversal |
| Developer tools | Install/rollback, packaging, manifest validation, diagnostics, logs and JUnit tests | Temporary fixtures; no clean second-PC installation |
| Live QA commands | All scenarios can be imported and display help without native tooling | Does not run the scenarios or prove their live results |

Run `python tools/framework.py test --shared-only` for asset-free checks, or the
ROM-backed command in [Development](../../docs/DEVELOPMENT.md) for the full suite.
Results include per-check logs and JUnit under `work/framework-build/test-results/`.
The full receipt records exact tested binary and asset hashes; the shared receipt
cannot authorize installation. CI uploads its logs, including failed checks.

## Historical live evidence

The detailed [2026-10-11 validation record](https://github.com/TedCassirer/mariosouls/blob/2efdcc6/framework/docs/VALIDATION.md)
is preserved in Git history, including measurements, failed setups, corrections
and explicit unverified cases. It describes the builds tested at that time.
Private captures and saves are not part of the repository.

| Area | Previously observed | Coverage still needed |
| --- | --- | --- |
| Asylum movement and interactions | Jumping, bonfire/Leave, bloodstain recovery and selected doorways on dedicated QA saves | Broader scripted interactions and physical-controller matrix |
| Asylum combat | Native HP/stamina changes, selected weapon/shield actions, critical attempts, fireballs and rewards | All weapon variants, enemies and encounter states |
| Death | Native lethal damage, visible Mario death presentation and respawn in a targeted test | Repeated deaths, hazards and loads across supported areas |
| Firelink doorway | The reported NPC-only collision barrier was crossed in both directions after the filter correction | Other barriers and area transitions |
| Cannon and kick impacts | Targeted records in the feature guides below | Later aim fixes, boss exclusions and contact behavior across the game |

These observations are not blanket passes for the current build. In particular,
the historical pit-protection claim was corrected after camera-only geometry
prevented lethal falls. Synthetic pit tests pass; the corrected live death and
respawn path still needs explicit validation. Area-transition combat and traction
also retain live coverage gaps documented in the historical record.

See [Cannon](CANNON.md), [Kick impacts](KICK_IMPACTS.md),
[Starting classes](STARTING_CLASSES.md) and [Debug overlay](DEBUG_VIEW.md) for
feature-specific scope and limitations.

## Release gates

The [release checklist](../../release/checklist.json) remains authoritative.
Outstanding gates include a clean second-PC install/uninstall, portable asset
setup, full-game regression, input/display combinations, save safety, loader
compatibility, exact-package distribution review and candidate sign-off.

Use the [live QA guide](../qa/README.md) and [testing matrix](../../docs/TESTING.md).
All automated control requires backups, an eligible disposable test character
and a current handoff. Never test on the owner's level-65 character.

When adding evidence, record the commit and binary hashes, fixture, area,
equipment, reproduction steps, expected and actual results, and relevant HP,
stamina, timing or movement measurements. Distinguish passed, failed and untested
cases. Update the coverage summary with concise findings; keep private traces
local and rely on Git history for superseded development notes.
