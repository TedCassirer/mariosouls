# Souls Character Framework 0.1

MarioSouls is the first character adapter for a shared Dark Souls PTDE integration.
This is a working extraction of the installed 0.7 integration, with a versioned
DLL interface, host-independent combat policy, manifest-driven attacks, a character
generator, conformance checks and a backed-up installer. It is not a universal
importer for arbitrary game models or ROMs.

| Directory | Responsibility |
| --- | --- |
| `sdk/` | Versioned C interface between the host and each character |
| `core/` | Attack windows, progression, exhaustion and validation |
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

See [the ABI contract](docs/CONTRACT.md), [adding a character](docs/ADDING_A_CHARACTER.md)
and [verification](docs/VALIDATION.md). New gameplay work follows the root
`AGENTS.md`. The old `work/` implementation is retained as history; its two build
entry points now delegate to this framework. The old checkpoint tag and runtime
archive are untouched.
