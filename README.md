# Souls Character Framework

Reusable character integration for Dark Souls: Prepare to Die Edition, with Mario 64
as its first adapter. Shared rendering, world collision, native interactions, input,
health, stamina and combat now live under `framework/`. Character engines and
animations live in separate adapter DLLs. Wind Waker Link now has a first playable
adapter with privately extracted assets and an authored controller. Link/OoT and
DK/DK64 are not implemented. See `framework/characters/link_ww/README.md` for scope.

Start with [the framework guide](framework/README.md),
[adding a character](framework/docs/ADDING_A_CHARACTER.md) and
[verification](framework/docs/VALIDATION.md). Future gameplay changes follow
[AGENTS.md](AGENTS.md). The `work/` prototype remains historical source.

Current local checkpoint: **framework 0.4.2 visual diagnostics**, tagged
`checkpoint-2026-10-11-framework-v0.4.2-debug`. F8 toggles the overlay, F9 selects
Combat/Terrain/Objects/All, and F10 controls through-walls drawing. See
[debug view meanings and limits](framework/docs/DEBUG_VIEW.md). With the game
closed, `outputs/Restore framework debug checkpoint.cmd` restores this installed
Mario version, including the 0.4.1 boss-contact and doorway fixes. Earlier restore
helpers intentionally return to their older versions of the entire framework.

The original playable **0.7 contextual interactions** checkpoint is preserved at
`checkpoint-2026-10-11-mariosouls-v0.7`. The instructions below still restore it.

## Return to the historical 0.7 version

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

- `framework/`: supported host, core, SDK, Mario adapter, templates and checks.
- `tools/framework.py`: build, test, doctor, install and new-character commands.
- `work/mariosouls.cpp`, related headers: preserved 0.7 implementation history.
- `work/build_sm64.py`, `work/build_plugin.py`: compatibility wrappers that now
  delegate to the framework, instead of compiling the old host.
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
python tools/framework.py doctor
python tools/framework.py build --with-runtime
```

Framework DLLs go to `work/framework-build`, including the selected character's
adapter and optional runtime in `characters/<id>`. These commands do not install
them. Run the framework's `test` then `install` commands documented in its guide.
For exact rollback,
use the captured runtime restore above instead of rebuilding. Historical installers
and source-transformation scripts are retained as development history; do not run them
as a generic installer because some target earlier versions.

The running game reads the user's US Mario 64 ROM from the configured local path.
Collision data was generated privately from the installed Dark Souls files using
`work/export_collision.py`. Neither input assets nor generated caches are tracked.
Optional live tests also require Universal Modder 0.2.0 and the local packages listed
in the dependency lock. New live helpers under `framework/qa/` require `Mario QA`,
level 1–10, in the Asylum and foreground. The host's bounded harness also permits
the old `test` character at level 1–10. Do not use the level-65 character.

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
