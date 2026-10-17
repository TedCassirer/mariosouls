# Reporting bugs

Open a report using the [GitHub bug template](https://github.com/TedCassirer/mariosouls/issues/new?template=bug_report.md). Reporting is manual; the mod does not upload your game state, open an in-game reporting bridge or record a session journal.

## Include useful context

- MarioSouls commit/build and PTDE version/fingerprint if known.
- Mario class/manual mode, equipped gloves/weapon and upgrade level when relevant.
- Area, starting position and a short sequence of actions.
- Expected behavior, actual behavior and whether it repeats.
- Controller or keyboard/mouse; DSfix version, FPS and scene/UI resolution.
- Other mods and whether F6/native mode has the same problem.

For collision/attack problems, F8 shows the overlay, F9 selects a view and F10 changes occlusion. Explain the approach direction; screenshots alone may not show a camera, timing or moving-platform issue. A short video can help.

## Logs and privacy

`DATA/mariosouls.log` contains bounded startup/error information and is replaced on the next launch. Copy the relevant excerpt before restarting. It is not a continuous input/movement record.

Review paths/player names before sharing logs or captures. Do not attach ROMs, saves, extracted game assets, binaries, memory dumps, credentials or complete private backups. For a sensitive security issue, follow [SECURITY.md](../SECURITY.md) instead of publishing the details.

## For a proposed fix

Link the issue/reproduction in the PR, update [Features and tweaks](FEATURES_AND_TWEAKS.md) and [CHANGELOG.md](../CHANGELOG.md), and describe what was actually compiled or checked in game. If developing in a fork, push the fix there and open/reuse a PR against the original repository. See [contributing](../CONTRIBUTING.md).
