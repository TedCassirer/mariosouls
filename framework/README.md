# Souls Character Framework 0.4

Wind Waker Link is the second real adapter, using an authored controller and
privately extracted original assets. Shared skeletal asset loading, offline
preparation, cutout materials and kinematic queries support future adapters.
See [Link's scope, preparation and limitations](characters/link_ww/README.md).
Link/OoT and DK/DK64 remain separate, unimplemented projects.

MarioSouls is the first character adapter for a shared Dark Souls PTDE integration.
This is a working extraction of the installed 0.7 integration, with a versioned
DLL interface, host-independent combat policy, manifest-driven attacks, a character
generator, conformance checks and a backed-up installer. It is not a universal
importer for arbitrary game models or ROMs.

| Directory | Responsibility |
| --- | --- |
| `sdk/` | Versioned C interface between the host and each character |
| `core/` | Movement ownership, lifecycle, input buffering, combat, feedback and validation |
| `hosts/ds1ptde/` | Native collision, rendering before UI, input, interactions, health, stamina and damage |
| `characters/mario64/` | Mario's engine, action IDs, animations, sounds and procedural flask |
| `templates/` | Compilable starting adapter for a future character |
| `tests/` | Shared policy and real DLL conformance tests |
| `qa/` | Guarded observation and controls for Mario QA, level 1–10, Asylum only |

Build from the repository root with the pinned dependencies installed:

```powershell
python tools/framework.py doctor
python tools/framework.py build --with-runtime
python tools/framework.py test --asset "C:\path\Super Mario 64 (USA).z64" --runtime "C:\absolute\project\work\framework-build\characters\mario64\sm64.dll"
python tools/framework.py install --asset "C:\path\Super Mario 64 (USA).z64" --runtime "C:\absolute\project\work\framework-build\characters\mario64\sm64.dll" --check-only
```

Close Dark Souls normally, then repeat `install` without `--check-only`. Builds
alone do not install anything. The installer requires tests for the exact host,
adapter, runtime and assets, checks the game executable and backs up every replaced
file. On a failed copy it restores those files. ROMs are referenced, never copied.

`build` without `--with-runtime` rebuilds the host and adapter only. `--adapter-only`
is useful for adapter development but intentionally cannot produce an installable
receipt: make a full build and run tests before installing. Build products and
private QA evidence go under `work/framework-build/`.

Installed adapters live under `DATA/SoulsCharacterFramework/characters/<id>/`.
`[Framework] Character` selects one adapter on startup; live swapping is not
supported. The existing `mariosouls.ini`, `mariosouls.log` and `MarioSouls/collision`
names are retained for compatibility with the user's controls and private caches.
The host does not interpret Mario engine actions. Those names are storage
compatibility, not separate implementations.

Balance overrides use `[Combat.<id>]`, including `BasePower`, `UpgradePower`,
`UpgradeLevel`, `FixedPower`, `StatScaling`, `<attack>_multiplier` and `<attack>_cost`.
Installation migrates the existing Mario `[Combat]` values once. Another character
starts with its manifest defaults. Upgrades remain configurable test values;
this refactor does not add an inventory/titanite upgrade shop.

Version 0.2 introduces a single-use native movement mailbox and explicit ownership
for character locomotion, native actions and loading/death recovery. Each guest
step waits for its previous request to be consumed; the native capsule position
is sampled before collision correction and combat. Position corrections preserve
animations and clip only momentum directed into confirmed native contact planes.
Queued input and movement cannot survive an ownership change. ABI 2 makes this
contract available to every adapter through `resolveMotion`; ABI 1 DLLs must be
rebuilt. See [movement ownership](docs/CONTRACT.md#movement-ownership-abi-2).

Shared impact feedback now adds a short attacker pause, procedural contact sound,
depth-tested contact burst and controller vibration after native target HP falls.
Attack manifest multipliers determine the weight of the response, so other
characters inherit it without host branches or SDK changes. Misses and contacts
that do not lower HP do not trigger this damage feedback. Native guard reactions
are left to the host. See [impact feedback](docs/FEEDBACK.md) for settings and limits.

See [the ABI contract](docs/CONTRACT.md), [adding a character](docs/ADDING_A_CHARACTER.md)
and [verification](docs/VALIDATION.md). New gameplay work follows the root
`AGENTS.md`. The old `work/` implementation is retained as history; its two build
entry points now delegate to this framework. The old checkpoint tag and runtime
archive are untouched.


Version 0.3 adds independent model/body sizing and a landing damage policy to the
shared descriptor (ABI 3). Mario's body profile reduces collision dimensions and
landing damage without changing his movement speed or jump height. See the
contract and validation report for exact dimensions and measured coverage.
