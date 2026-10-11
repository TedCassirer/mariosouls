# Recovery

Git is the source recovery system. There is no separate source-checkpoint command.
Use normal commits for ongoing work and tags for named milestones.

## Recover source

Find an earlier commit with `git log --oneline`. Inspect it in a separate checkout:

```powershell
git worktree add --detach ..\mariosouls-recovered <commit-or-tag>
```

Use `git revert <commit>` to undo a committed change on the current branch while
preserving history. Earlier source still needs its pinned dependencies and your
own game assets to build. Existing historical checkpoint tags remain unchanged.

## Restore an installation

Git does not store compiled mod files, local settings or generated game data.
The installer therefore automatically backs up files it replaces under
`work/install-backups/`. To restore the previous installation, close Dark Souls:

```powershell
python tools/framework.py uninstall --check-only
python tools/framework.py uninstall
```

This verifies file hashes and restores one installation transaction. It preserves
unrelated files and saves, and refuses to overwrite files changed since installation.
See [troubleshooting](TROUBLESHOOTING.md) for details.

## Historical local backups

Existing archives, manifests and tags are preserved. Older installation backups
under `work/checkpoints/` remain supported when they contain hash inventories.
The original workspace also retains its historical restore helpers in `outputs/`,
with the retired 0.7 restore tool under `work/legacy-checkpoint-tools/`. Those
machine-specific files are not distributed in new source packages. A fresh clone
cannot restore archives it does not have.

Keep private backups on a separate drive. Avoid `git clean -fdx` in a prepared
workspace: it deletes ignored assets, dependencies and backups. Installation
backups do not replace separate save backups.
