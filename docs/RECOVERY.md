# Recovery

Source history, installed files and player saves are separate things. Git contains source; installation snapshots contain replaced local files; saves need their own backups.

## Undo a managed installation

Close the game and run from the project folder:

```powershell
.\MarioSouls.ps1 uninstall
```

Use `-GameDirectory` again for a nondefault installation. The setup record and original backups under `DATA/SoulsCharacterFramework/setup/` must remain together. Uninstall removes managed mod DLLs, restores eligible original settings and reverses the owned DSfix wrapper change. Edited config files and unrelated DSfix settings are protected as described in [installation](INSTALLING.md).

Changed managed binaries or another wrapper chain stop recovery rather than deleting somebody else's files. A missing setup record does not authorize guessing how to remove a manual/older install.

## Interrupted or manual installation

Each managed operation keeps a small pre-operation snapshot. Replacement errors attempt immediate rollback; after a process interruption, inspect the relevant snapshot and restore only the files involved, with the game closed. Keep the corresponding record/config and do not apply an old revision's restore tool blindly to a later setup.

For manual copies, restore the exact loader/config bytes you backed up and remove only files you recorded as newly added. With DSfix, restore its wrapper arrangement too. Keep ROMs, generated collision/props, saves and historical archives.

## Return to earlier source

Commits and existing tags are the source checkpoint system. Inspect an earlier revision without disturbing current work:

```powershell
git worktree add --detach ..\mariosouls-recovered <commit-or-tag>
```

Use `git revert <commit>` to undo a committed change while preserving history. Earlier revisions still require their own pinned dependencies and owned assets; a source checkout does not restore an installed game or save.

Preserve existing checkpoint tags and local archives. Do not run broad cleanup over ignored files: `work/` and private output directories hold dependencies, assets, captures and backups that Git cannot recover.
