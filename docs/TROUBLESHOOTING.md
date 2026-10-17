# Troubleshooting

Start with the exact build, game location and whether the problem also occurs after switching to native mode with F6. Keep a copy of relevant `DATA/mariosouls.log` excerpts before another launch; the log is replaced each time and capped at 256 KiB.

## Setup refuses to install

- Close the actual `DARKSOULS.exe` process normally. A closed window alone does not establish process exit.
- Check `-GameDirectory`: it must identify the supported PTDE installation, not Remastered or an unrelated private copy.
- Missing Python/build dependencies or ROM/collision/equipment paths require the [first-time preparation](DEVELOPMENT.md).
- An unknown `dinput8.dll`, another DSfix wrapper or a changed managed binary is deliberately protected. Resolve the loader/file ownership conflict rather than forcing a copy.
- A stale `-WhatIf` preview stops without building. A real install rebuilds current source automatically.

Use the same game location for install/update/uninstall. Keep the managed setup record and its backups together. See [installation](INSTALLING.md) and [recovery](RECOVERY.md).

## Mario is missing or a human appears

Confirm the saved class is Mario/Pyromancer or use F6. Check the ROM/runtime/adapter paths in `mariosouls.ini`, matching x86 DLLs and the current map's collision export. A refused hook, invalid actor or failed replacement draw should retain native presentation rather than hide the player entirely.

Report whether it occurs on initial load, death, a seamless area change, a specific cutscene or only one resolution. A class preview and a playable guest instance have different lifetimes. [Class integration](../framework/docs/STARTING_CLASSES.md) explains the intended behavior.

## Movement, interactions or attacks fail

Use F8 and the relevant F9 view when capturing the location. The overlay's capsules and cached terrain are diagnostic approximations, not every native collision shape.

For door/stair/lift problems, note the precise approach and whether native mode passes. For A/Estus problems, include grounded state, an open menu/dialogue and any concurrent damage. For unarmed damage, confirm normal Mario's Gloves are in the active right hand and stamina is sufficient. Record the glove/weapon upgrade and whether the issue begins after an area change.

Do not change arbitrary memory flags or assume a missing guest-floor triangle means native collision should be removed. Preserve the state long enough to make a useful [bug report](BUG_REPORTING.md).

## DSfix, frame rate or audio differs

Confirm the supported loader chain and distinguish scene resolution from UI/backbuffer resolution. Mario simulates at 30 Hz; interpolation only smooths presentation. Include the FPS limit, AA/motion-blur settings, other wrappers/HUD mods and whether the problem remains without DSfix.

For enemy/boss audio, record the encounter and compare Mario/native mode. Mario audio and native game audio are separate paths; a successful audio API call does not establish that both are audible.

## Steam still shows the game running

Distinguish a surviving windowless `DARKSOULS.exe` from Steam retaining a terminated PID. Steam's `logs/gameprocess_log.txt` records process IDs/exits. Historical local investigations reproduced a stale running flag with vanilla PTDE and a separate Steam-only program; that does not establish the cause of every shutdown issue.

Save and quit normally, preserve relevant logs and report whether the process still exists before restarting Steam or Windows. Do not force-close a process that may be saving. The repository no longer maintains special Steam-free development copies.

For anything unresolved, use the [manual bug-report guide](BUG_REPORTING.md) with build/settings and reproduction steps. Keep ROMs, saves and personal paths out of public evidence.
