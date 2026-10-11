# Troubleshooting

**This is a development build.** Start with [setup](DEVELOPMENT.md), and preserve
separate save backups before installing or debugging.

## Setup and build

Run `python tools/framework.py doctor`. Dependency setup verifies pinned revisions
and refuses unexpected local edits. Restore or inspect those edits rather than
forcing a reset. Compiler download verifies both archive and executable hashes.
A partial download is preserved on the next attempt; inspect it before removing it.

Set `MARIOSOULS_GAME_DIR` to the directory containing `DARKSOULS.exe` before running
the supported build/install tooling when Steam is elsewhere. Some historical live
QA scripts still use machine-specific paths; inspect them separately.

## Install refused

The game must be closed. A full ROM-backed test receipt must match the exact host,
adapter, runtime, assets and sources. Rebuild and rerun full tests after changes.
Shared CI does not satisfy this requirement. Unknown loaders require conflict
review; see [compatibility](COMPATIBILITY.md).

Do not copy a ROM or extracted game data into the repository to fix missing assets.
Point setup at your locally owned files instead.

## Restore one installation

```powershell
python tools/framework.py uninstall --check-only
python tools/framework.py uninstall
```

Close the game first. This restores files replaced by the most recent installation;
if that installation replaced an older mod, the older mod is restored. It is not
necessarily a return to vanilla. Unrelated files and saves are untouched.

Restoration requires the matching private backup beneath `work/install-backups/` (or `work/checkpoints/` for older installs).
Current and backup file hashes are verified. Modified files cause refusal: preserve
those changes and review the backup before proceeding. There is no force-delete
option. Pre-existing backups without hash inventories use their historical restore
helpers; see [recovery](RECOVERY.md). Keep backups when moving the checkout.

## Reporting a bug

Include game revision, mod commit, map, input device, relevant settings and short
reproduction steps. Describe what happened and what you expected. Review logs and
captures for personal data; never upload ROMs, saves or extracted assets.
