# MarioSouls

Local development project for the Dark Souls: Prepare to Die Edition Mario integration.
This repository starts with the installed **0.7 contextual interactions** checkpoint,
tagged `checkpoint-2026-10-11-mariosouls-v0.7`.

## Return to this playable version

1. Quit Dark Souls normally and close the game.
2. Run `outputs/Restore MarioSouls checkpoint.cmd`.

The restore checks the local archive and all 72 files against the committed SHA-256
manifest. It requires the same Dark Souls executable, preserves the runtime it replaces
under `work/checkpoints/before-restore-*`, and restores the captured mod DLLs,
configuration, collision caches, installed source/tools and title-movie setup.
It does not change character saves or the Mario ROM. It does not delete unrelated
files added after this checkpoint.

To check the archive without restoring anything:

```powershell
powershell -NoProfile -File "outputs/Restore MarioSouls checkpoint.ps1" -CheckOnly
```

The exact runtime cache is `work/checkpoints/mariosouls-v0.7-runtime.zip`.
It contains private game-derived caches and is deliberately **not in Git**. Keep it
with this project or copy it separately to a backup drive. A Git clone or bundle alone
does not contain that archive, the ROM, saves, toolchain or upstream dependency trees.
Do not use `git clean -fdx` in this workspace: it would remove these local dependencies
and backups. `Restore original game.cmd` is a different operation: it disables the mod.

## Repository contents

- `work/mariosouls.cpp`, related headers: native host integration, rendering, input,
  combat, interactions, world collision and test bridge.
- `work/build_sm64.py`, `work/build_plugin.py`: existing 32-bit Windows build scripts.
- `work/combat-harness`, `work/progression`, `work/interaction-v2`,
  `work/world-bridge`, `work/ui-render`: development and verification scripts.
- `patches/libsm64.patch`, `patches/libsm64/host_pose.h`: our complete local libsm64 changes.
- `dependencies.lock.json`: exact upstream revisions, changed-source hashes and compiler hash.
- `checkpoints/`: captured configuration, installed manifest and runtime archive inventory.
- `outputs/`: controls, validation reports and launch/restore helpers. Videos and large
  captures remain local and ignored.
- `work/MODLOG.md`: chronological engineering record; older entries describe older builds.

The allowlist in `.gitignore` excludes ROMs, saves, extracted game assets, memory dumps,
upstream source checkouts, Python packages, compilers, binaries and recordings.
When adding a new source directory, update the allowlist and check `git status`.
Development scripts may contain machine-specific paths. This is a local checkpoint,
not a portable public mod release. No remote is configured.

## Recover development source

To inspect the checkpoint while retaining later development, run from this repository:

```powershell
git worktree add ..\mariosouls-recovered checkpoint-2026-10-11-mariosouls-v0.7
```

The recovered tree contains all authored integration sources and patches. Run
`python tools/dependencies.py --setup` inside it to clone the pinned upstream
libsm64 and MinHook revisions and apply our changes. Existing modified dependency
checkouts are never reset by this helper; it stops if they differ.

For this existing workspace, `python tools/dependencies.py` verifies dependencies
without changing them. Libsm64's seven modified/new files are captured in the patch
and overlay, rather than relying on the uncommitted state of the nested checkout.

## Build

Use Python 3.12 and Zig 0.13.0 for Windows x86_64, targeting `x86-windows-gnu`.
The current compiler and original archive are retained under `work/tools`; their
hashes and locations are recorded in `dependencies.lock.json`. For a recovered tree,
copy/extract that compiler at the same relative path, then:

```powershell
python tools/dependencies.py
python work/build_sm64.py
python work/build_plugin.py
```

Both DLLs go to `work/build`. These commands do not install them. For exact rollback,
use the captured runtime restore above instead of rebuilding. Historical installers
and source-transformation scripts are retained as development history; do not run them
as a generic installer because some target earlier versions.

The running game reads the user's US Mario 64 ROM from the configured local path.
Collision data was generated privately from the installed Dark Souls files using
`work/export_collision.py`. Neither input assets nor generated caches are tracked.
Optional live tests also require Universal Modder 0.2.0 and the local packages listed
in the dependency lock. The existing combat harness permits only character `test`
in the Asylum; `Mario QA` setup is pending. Do not loosen the guard or use the level-65
character to make a test pass.

## Checkpoint status

Verified in game: direct Estus and native healing/charges, bonfire rest/leave/jump,
ladder travel/pause/dismount, locked-door response, menus, rejected action recovery,
bloodstain recovery, incoming/outgoing damage and death/respawn. Mario stays instantiated
through contextual actions. The prior world-collision and UI-layering fixes are included.

Fresh NPC conversation remains unverified. The physical controller was disconnected
during the latest pass; its routing has executable policy checks, but no new physical
controller confirmation. This tag preserves the tested state, not a claim that every
area, item or scripted event works. See `outputs/MarioSouls interaction update.txt`.

Checkpoint tooling verification: runtime hashes match the installed files, dependency
hashes match the active source, and five isolated restore tests pass (corruption/path
rejection, process guard, backup/save preservation and rollback on failure).

Upstream dependencies retain their own licenses. No ROM, native game executable,
third-party decompiled source tree or personal save is committed here.
