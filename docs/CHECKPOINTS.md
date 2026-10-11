# Checkpoints and local backups

Git checkpoints preserve source. Runtime backups preserve a local installation.
They are different recovery mechanisms.

## Recover source

List preserved checkpoints:

```powershell
git tag --list "checkpoint-*"
```

Inspect one in a separate worktree without resetting your current changes:

```powershell
git worktree add ..\mariosouls-recovered checkpoint-2026-10-11-mariosouls-v0.7
```

The tags are historical milestones, not current recommended releases. Existing
checkpoint tags and archives must remain immutable. New source still needs its
dependencies and user-supplied assets to build.

## Restore a local runtime

`checkpoints/` contains manifests. Historical restore helpers in `outputs/`
require private archives under `work/checkpoints/`; those archives are **not on
GitHub**. A fresh clone cannot run these restore operations on its own.

For the original local 0.7 checkpoint, close Dark Souls and check the archive:

```powershell
powershell -NoProfile -File "outputs/Restore MarioSouls checkpoint.ps1" -CheckOnly
```

Only on the original prepared workspace, repeat without `-CheckOnly` to restore.
Other helpers restore their named historical versions, including their old
configuration. They are not generic installers or a substitute for save backups.

Keep private archives on a separate backup drive. Do not run `git clean -fdx` in
the prepared workspace: it would delete ignored dependencies, assets and backups.
