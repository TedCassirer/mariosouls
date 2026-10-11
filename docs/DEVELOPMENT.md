# Development setup

This is a Windows development workflow. A fresh clone is not yet a one-command
setup: external tools and game-derived data must be prepared locally.

## Prerequisites

- Git and Python 3.12.
- Zig 0.13.0 for Windows x86_64, targeting `x86-windows-gnu`.
- The supported Dark Souls: Prepare to Die Edition executable.
- Your own US Super Mario 64 ROM for the Mario adapter.

Exact dependency revisions, compiler hashes and optional Python packages are in
[`dependencies.lock.json`](../dependencies.lock.json). The compiler must be at
`work/tools/zig-windows-x86_64-0.13.0/zig.exe`. Python extras used for asset
preparation and QA are currently expected under `work/python-deps/`.

The tooling still has local path assumptions. In particular, the default game
directory is defined in `tools/framework.py`; live QA helpers have their own
paths. Review these before using the tools on another computer. A portable setup
and installer remain planned work.

## Dependencies and build

From the repository root:

```powershell
python tools/dependencies.py --setup
python tools/framework.py doctor
python tools/framework.py build --with-runtime
```

Dependency setup clones pinned upstream repositories and applies our patches. It
does not download the compiler or game assets and stops if existing dependency
edits differ. Runtime preparation operates on a staged copy of the dependency.

Build output is in `work/framework-build/`. Later builds without `--with-runtime`
rebuild the host and adapter, reusing the existing runtime.

## Tests

```powershell
python tools/framework.py test --asset "C:\path\Super Mario 64 (USA).z64" --runtime "work/framework-build/characters/mario64/sm64.dll"
```

The suite covers shared policies, native hook contracts, rendering and the real
adapter/runtime. It records a receipt for the exact tested files. Live gameplay
checks are separate; record them in the [validation log](../framework/docs/VALIDATION.md).

Live automation must use a dedicated test character and the existing guards.
Never test against the owner's level-65 character. Current QA helpers require
Mario QA at a low level or the exact permitted melee fixture; accepted maps vary
by helper. Read the helper before driving the game.

## Local game data

Collision and equipment data are generated from the installed game, not stored
in this repository. These commands each create a new private output directory:

```powershell
python tools/framework.py prepare-collision --map m18_01_00_00 --output work/my-asylum-collision
python tools/framework.py prepare-equipment --output work/my-equipment
```

The collision example prepares one map only. It is not a full-game cache, and
unprepared maps require other locally prepared data. Native event-controlled
colliders stay native-owned. Equipment preparation does not establish combat
coverage for every exported weapon.

## Installation in a development environment

Close the game. Validate the installation first:

```powershell
python tools/framework.py install --character mario64 --asset "C:\path\Super Mario 64 (USA).z64" --runtime "work/framework-build/characters/mario64/sm64.dll" --collision-directory "work/my-asylum-collision" --equipment-directory "work/my-equipment" --check-only
```

After reviewing the check, repeat without `--check-only`. The installer checks
the executable, test receipt and assets; backs up replaced files; and rolls back
failed copies. Keep a separate save backup as well. This command is a development
tool, not the planned public installer.

## Making changes

Follow [AGENTS.md](../AGENTS.md). Shared behavior belongs in `framework/core/`,
native integration in `framework/hosts/ds1ptde/`, and character behavior in its
adapter. Extend the SDK rather than adding character switches to the host.

Use the [adapter guide](../framework/docs/ADDING_A_CHARACTER.md) for a new character
and the [Link guide](../framework/characters/link_ww/README.md) for that adapter's
separate asset pipeline. Do not continue gameplay development in `work/`.
