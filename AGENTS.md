# Working on MarioSouls

## Project purpose and current status

MarioSouls replaces the Pyromancer class in Dark Souls: Prepare to Die Edition with Mario, using libsm64 movement and animation. It is implemented as a character framework so a future host edition or another guest character can reuse the integration work.

PTDE on Windows/x86 is the only implemented host. Mario is the only implemented character. Focused gameplay checks have been performed with stock rendering and DSfix 2.4, but compilation and historical observations do not establish full-game compatibility. Read [compatibility](docs/COMPATIBILITY.md) before making support claims.

Dark Souls Remastered is an architectural target, not a functioning port. Preserve the renderer boundary, explicit host capabilities, fixed-step timing, portable types and x86/x64 compilation checks. There are no Remastered addresses, hooks or DX11 renderer to extend yet. Link, DK and other characters need their own adapter, engine integration and user-supplied assets; a mesh or scaffold is not a character implementation.

## Read first

- [Development](docs/DEVELOPMENT.md): commands, dependencies and local assets.
- [Framework architecture](framework/README.md): responsibilities and current limits.
- [SDK contract](framework/docs/CONTRACT.md): lifecycle, units and ownership.
- [Features and tweaks](docs/FEATURES_AND_TWEAKS.md): current behavior and defaults.
- [Changelog](CHANGELOG.md): meaningful changes by development version.
- [Host porting](framework/docs/HOST_PORTING.md) and [PTDE integration inventory](framework/hosts/ds1ptde/PORT_CHECKLIST.md).

## Where code belongs

| Location | Owns |
| --- | --- |
| `framework/hosts/ds1ptde/` | Native addresses, structures, hooks, animation/equipment IDs, maps, collision, input, menus, health, stamina, damage, D3D9 and PTDE installation/assets |
| `framework/core/` | Character-independent lifecycle, movement/combat policies, timing, validation and host capability requirements |
| `framework/sdk/` | Neutral, versioned contracts between host and adapters; internal renderer interface |
| `framework/characters/<id>/` | Guest APIs, action/animation IDs, movement rules, asset decoding, meshes, props and character-specific fitting |
| `tools/` | Shared build, pinned dependency checks, profile generation and asset recipe dispatch |

All new gameplay integration belongs to `framework/`. Share capabilities through the SDK instead of adding character-name switches to the host. A Mario-only fix belongs in Mario's adapter unless the underlying responsibility is native or shared.

`work/` contains private dependencies, generated data, build outputs and preserved prototype/investigation history. Do not implement new gameplay there. Keep existing checkpoint tags and local archives immutable. Do not clean ignored files indiscriminately.

## Gameplay invariants

- Native Dark Souls HP, stamina, inventory, equipment, status effects, quest state, deaths and rewards are authoritative. Adapters cannot read or write game memory.
- Never grant native protection from an SM64 action flag. Ordinary jumps do not imply invulnerability. Guest and native collision must not charge the same hit twice.
- Preserve Mario's movement speeds and jump distances unless a requested gameplay change explicitly alters them. Model/body scale, equipment and presentation have separate roles.
- Native physics owns resolved movement, active doors/props and moving platforms. A cached guest floor must not recreate removed native geometry.
- Keep presentation separate from locomotion ownership. Menus, interactions, recovery, death and cutscenes can own native motion while a validated guest mesh remains visible.
- Rendering must not step gameplay. Preserve 30-Hz simulation, native acknowledgement/backpressure and render-only interpolation.
- Optional capabilities must fail predictably. Missing support must not invent damage, items, floors, quest effects or rendering above the HUD.
- Preserve executable, hook-signature, pointer, buffer and asset-format validation. These are production safeguards, not removable test scaffolding.

## Build and installation workflow

Use `python tools/framework.py doctor`, `build`, `build --with-runtime`, `build --check-portability` and `new-character` as documented. Prepare pinned dependencies with `python tools/dependencies.py --setup --download-toolchain` when needed.

`build --check-portability` compiles/links PTDE, Mario and its runtime for x86, checks C/C++ SDK/shared declarations for x86/x64, and builds the x64 guest components. It executes no check program and does not provide a 64-bit PTDE or Remastered host.

Automated verification is compilation only. Do not recreate behavioral suites, test labs, command bridges, replay systems, cheats, save editors, special development copies or background game control without an explicit request. Optional pose-authoring tools are asset tools, not regression suites.

Source installation uses `MarioSouls.ps1 install` / `uninstall`. PTDE's `installer.cs` player front end and `player_setup.py` source-build/asset bootstrap reuse the same `setup.py` for game changes and recovery. Do not duplicate that implementation or add installation to the shared build CLI. Builds alone never install, launch or control the game. Do not install a prepared change unless the current task authorizes deployment.

Build player candidates with `python tools/build_installer.py` from committed source. Review `release/installer.lock.json`, keep outputs private, and follow [release preparation](docs/RELEASING.md). Do not bundle a ROM, extracted game data or prebuilt guest geometry/runtime in this source-based installer. Verify actual installation/removal separately from compilation and keep publication a separate human decision.

## Required documentation updates

Treat documentation as part of the change, not a follow-up task.

1. For every gameplay feature, fix, tuning change or default change, update the relevant entry in [docs/FEATURES_AND_TWEAKS.md](docs/FEATURES_AND_TWEAKS.md). State the resulting behavior, important values/units, configuration keys, implementation location, known limitations and verification status. Update the existing entry when behavior changes instead of leaving contradictory descriptions.
2. Add a concise entry under the current unreleased version in [CHANGELOG.md](CHANGELOG.md) for meaningful gameplay, architecture, build, installation or documentation changes. Do not invent release dates or label existing behavior as newly implemented.
3. Update README/player guides when installation, class selection, controls or visible behavior changes. Keep README focused on what players need; put exhaustive tuning and native internals in the detailed guides.
4. Update `framework/docs/CONTRACT.md` and the adapter guide when SDK semantics, layouts or capabilities change. Preserve explicit version/size/calling-convention checks.
5. Update host-porting documents and the PTDE integration inventory when a native boundary changes. Record its purpose, inputs, outputs and verification method for a future Remastered implementation.
6. Distinguish implemented behavior, historical gameplay evidence, current compilation checks and unverified/proposed behavior. Never claim a stub or a successful compile proves a playable port or a working game interaction.

## Manual verification and private data

Use disposable test characters. Never test on the user's level-65 character. Agent-driven game input requires a current control handoff; previous permission is subject to the user's latest instructions. Follow the [manual checklist](docs/TESTING.md), record what was checked and identify remaining gaps.

Keep ROMs, saves, original/extracted game assets, generated collision/props, binaries, private captures, credentials and dependency checkouts out of Git. Do not publish unreviewed local logs or absolute personal paths. Preserve separate save backups and existing installation recovery data.

## Forks, fixes and pull requests

The canonical upstream is [TedCassirer/mariosouls](https://github.com/TedCassirer/mariosouls).

Before delivering a fix, inspect Git remotes and repository metadata to determine whether the working repository is a fork. Do not assume that `origin` is the canonical upstream or invent a repository owner/base branch.

When working in a fork, commit the fix on a focused branch, push that branch to the fork and create a pull request against the original repository's appropriate base branch. Reuse an existing relevant PR rather than opening duplicates. Never push the fix directly to the original repository merely because an upstream remote is configured. Include the resulting PR URL in the handoff and attach it to the task when supported by the client.

The PR must explain the problem, resulting behavior, compilation performed, manual evidence and unverified behavior, and include the required documentation updates. Respect an explicit instruction to keep work local or prepared. If push/PR creation is blocked by credentials or connectivity, report exactly what remains instead of claiming delivery. Creating a PR does not imply merging or publishing a release.
