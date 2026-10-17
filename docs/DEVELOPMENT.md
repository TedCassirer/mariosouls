# Development

The framework has two goals: maintain the playable PTDE Mario integration, and keep edition/character boundaries usable for future work. Remastered preparation is structural; no Remastered host exists. Read [architecture](../framework/README.md) and [AGENTS.md](../AGENTS.md) before changing integration code.

The detailed [architecture specification](../framework/docs/ARCHITECTURE.md) defines intended dependencies/authority and known implementation debt. For every framework change, identify its owning layer, input/output/lifetime, capability/eligibility and failure path before implementing it. Use the placement examples and include that boundary explanation in review. An existing shortcut is not permission to spread it; no new architecture-test platform is implied.

## Build from source

Use Windows, Git and Python 3.12. The pinned Zig 0.13.0 toolchain and upstream revisions are recorded in [dependencies.lock.json](../dependencies.lock.json).

```powershell
python tools/dependencies.py --setup --download-toolchain
python tools/framework.py doctor
python tools/framework.py build --with-runtime
```

Dependency preparation verifies revisions/hashes and refuses unexpected local modifications. Runtime preparation stages a private source copy and applies the adapter's extensions there, leaving pinned checkouts intact. It never downloads a ROM.

| Command | Use |
| --- | --- |
| `build` | Build the x86 host and adapter; reuse an existing guest runtime |
| `build --with-runtime` | Also rebuild the x86 guest runtime |
| `build --check-portability` | Full x86 build plus x86/x64 C/C++ SDK/core declarations and x64 guest compilation |
| `doctor` | Check pinned dependencies, manifest validation and architecture boundaries |
| `new-character <id> --name <name>` | Generate an explicitly nonfunctional adapter scaffold |

Use the runtime/portability build after changing guest source or patches. Compile-only checks do not execute a guest or game and do not provide Remastered support. CI performs dependency preparation and `build --check-portability`; it retains build logs, not gameplay certificates.

## Outputs and provenance

```text
work/framework-build/dinput8.dll
work/framework-build/characters/mario64/character.dll
work/framework-build/characters/mario64/sm64.dll
work/framework-build/characters/mario64/build.json
work/framework-build/portability/...
```

The manifest records version, source revision/dirty state, source fingerprints and binary hashes. Never install x64 portability outputs into PTDE. Builds alone do not install, launch or control a game. [Installation](INSTALLING.md) is a separate one-command operation.

`--host <id>` selects the edition's actual build/link recipe and architecture. Only `ds1ptde` is implemented; unknown/unimplemented hosts are refused before building. PTDE paths above remain unchanged. Future hosts build under `work/framework-build/hosts/<id>/`, with their own adapter/runtime and manifest; the builder inspects DLL machine types. Asset commands also dispatch edition-owned recipes/default game paths and accept `--game`. PTDE installation/executable guards are unchanged. Follow [host porting](../framework/docs/HOST_PORTING.md) for the presentation-only first milestone and separate later gameplay integration.

## Owned assets

Playing requires a locally supplied US big-endian SM64 ROM and native PTDE exports. Asset tools need `requirements-assets.txt`, Soulstruct and its map-collision tooling. Existing private setups can use `work/soulstruct-source/src` and `work/python-deps`. These optional packages are not needed for compilation and are not installed silently by setup.

For a nonstandard game location, set this before running recipes:

```powershell
$env:MARIOSOULS_GAME_DIR = 'D:\Games\Dark Souls\DATA'
```

Use fresh private output directories:

```powershell
python tools/framework.py prepare-collision --map m18_01_00_00 --output work/asylum-collision
python tools/framework.py prepare-equipment --output work/equipment
```

Collision preparation exports one map per invocation. Export each required map separately; the installed collision directory must contain the map files for the areas used. Retain original exports/manifests as provenance when collecting map files into one directory. Existing dynamic Asylum-floor data must be preserved. Equipment export emits native held props and a manifest. No ROMs, collision, props, previews or original game files belong in Git.

Adapter-owned `prepare-assets` / `preview-assets` recipes are optional. `preview-equipment` is a manual pose-authoring workflow, not a gameplay regression suite. See [equipment authoring](../framework/characters/mario64/EQUIPMENT.md) and [tool reference](../tools/README.md).

## Making and delivering changes

Keep native logic in the edition host, neutral policies in core/SDK and guest-specific behavior in its adapter. Preserve the existing renderer/timing/capability boundaries; do not add speculative Remastered hooks or a host switch for another character.

For behavior/tuning changes, update [Features and tweaks](FEATURES_AND_TWEAKS.md), the unreleased [changelog](../CHANGELOG.md), and the relevant player/SDK/porting guide. Record compilation separately from manual evidence. Optional source formatting uses `requirements-dev.txt` and `python tools/framework.py format`; formatting is not a CI gate.

Use disposable test characters and a current control handoff for agent-driven input. Never use the owner's level-65 character. Automated gameplay suites, labs, cheats, replay and background game-control infrastructure are outside the current workflow.

When working in a fork, push the focused fix branch to that fork and open/reuse a PR against the original project. Confirm remotes/base branch first. Follow [CONTRIBUTING.md](../CONTRIBUTING.md) and include the required documentation updates.
