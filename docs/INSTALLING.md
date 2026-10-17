# Installation

MarioSouls has a Windows installer candidate and a source-checkout workflow. Both use the same PTDE backend for executable validation, DSfix handling, backups and removal. Remastered is not supported.

## Windows installer candidate

`MarioSouls-<version>-Setup.exe` is an experimental installer preview, not a fully verified stable release. Download it only from this project's [GitHub Releases](https://github.com/TedCassirer/mariosouls/releases), and read the verification limits in its release notes. Releases in a private repository require repository access.

1. Close Dark Souls. Open the installer on Windows 10/11 x64 with .NET Framework 4.8.
2. Choose PTDE's `DATA` folder and your own US SM64 8 MiB big-endian `.z64` ROM.
3. Click **Install / Update**. The first run downloads pinned Git, Zig and Python packages, builds the mod, and exports maps/equipment from your game. Internet, several GB of free space and several minutes are required; no preinstalled Python, Git or compiler is needed.
4. Launch normally through Steam. The same executable's **Uninstall** button removes the managed setup without needing the ROM, compiler or network.

The installer does not download a ROM or Dark Souls, elevate itself, or change Windows policy. For a protected game folder, run it as administrator. It is unsigned; Windows may show an unknown-publisher warning. Compare the published SHA-256 before running it.

Source, tools, exports and the last `setup.log` stay under `%LOCALAPPDATA%/MarioSouls/setup/<source-commit>/`. Keep this cache while installed: configuration points to its exported data. Keep the selected ROM at its chosen path. Uninstall retains this private cache and all saves.

See [release preparation](RELEASING.md) for verification limits. The optional legacy Asylum arena-floor file remains on existing installations; it is not bundled with the candidate. The source-checkout workflow below requires Python 3.11+ and prepared dependencies/assets as described in [development](DEVELOPMENT.md).

Unsupported Havok collision shapes, including some Ash Lake shapes, remain native-only and are recorded in the export manifest. Their Mario movement/support has not been verified in game. The candidate is not a claim of full-game traversal compatibility.

## Install, update or uninstall

Close Dark Souls normally. Open PowerShell in the project folder and run:

```powershell
.\MarioSouls.ps1 install
```

Use the same command to update; stale source builds are rebuilt through the framework build tool. Existing gameplay tuning and asset paths are retained. Repeating an already-current install does not create another recovery snapshot.

To remove the setup managed by this installer:

```powershell
.\MarioSouls.ps1 uninstall
```

These are the source-checkout commands. The PowerShell entry point and Windows front end share the PTDE host's `setup.py` implementation of game-file modifications.

## Game and ROM locations

The default game directory is:

```text
C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA
```

That directory contains `DARKSOULS.exe`. `-GameDirectory` also accepts its parent directory containing `DATA`. The installer accepts only the supported executable fingerprint in [edition.json](../framework/hosts/ds1ptde/edition.json).

Your US SM64 ROM can stay anywhere. It must be an 8 MiB big-endian `.z64` file; setup checks its size/header/region. It does not copy or download the ROM. Existing installs use `[Character.mario64] Asset` in `DATA/mariosouls.ini`, falling back to the older ROM setting when present.

For a first setup or changed locations, supply paths in one command:

```powershell
.\MarioSouls.ps1 install -GameDirectory 'D:\Games\Dark Souls\DATA' -Rom 'D:\Private\Super Mario 64 (USA).z64' -CollisionDirectory 'D:\Private\collision' -EquipmentDirectory 'D:\Private\equipment'
```

`CollisionDirectory` must contain the exported map `.bin` files, and `EquipmentDirectory` the native `.scprop` exports. Setup does not generate them. Export manifests are checked when present; legacy collision directories without a manifest receive format checks only. Keep the optional existing `MarioSouls/collision/asylum-arena-floor.bin` used by the dynamic Asylum floor.

The ROM/asset paths are saved in the game's configuration. Use `-GameDirectory` again for update/uninstall when the game is outside the default location, or set `MARIOSOULS_GAME_DIR` for the shell session. `MARIOSOULS_PYTHON` / `-Python 'C:\path\python.exe'` select a Python interpreter when necessary.

## With or without DSfix

Both stock rendering and DSfix 2.4 have had focused development checks. DSfix is installed separately and is not bundled.

| Setup | Loader arrangement |
| --- | --- |
| Without DSfix | MarioSouls host becomes `DATA/dinput8.dll` |
| Supported DSfix 2.4 | Keep DSfix's `dinput8.dll`; install `MarioSouls-host.dll` and set DSfix's wrapper to that file |

The script recognizes DSfix by its verified hash, preserves shaders and all unrelated settings, and leaves the chosen FPS/resolution settings intact. Native anti-aliasing and motion blur should be disabled for DSfix. See [compatibility](COMPATIBILITY.md) for the checked configurations and limits.

Unknown loaders, another wrapper chain, changed managed binaries, an unsupported executable or a running game stop setup before replacements. Resolve such conflicts rather than forcing an overwrite.

## Preview and PowerShell permissions

```powershell
.\MarioSouls.ps1 install -WhatIf
```

Preview validates and lists planned file operations without building, backing up or changing game files. It reports a stale build instead of building it; a real install handles the rebuild.

If Windows blocks this unsigned local script, inspect the source first. A process-local policy override can run the same command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\MarioSouls.ps1 install
```

The script does not change the machine's execution policy or elevate itself. Use a shell with write permission to the game directory when required.

## Backups and uninstall behavior

The setup record and recovery snapshots are under `DATA/SoulsCharacterFramework/setup/`. Keep that directory until uninstall is complete. Replacement errors trigger immediate rollback; snapshots support manual recovery after an interrupted process.

Uninstall removes managed mod DLLs, restores the prior configuration unless you subsequently edited it, and restores DSfix's previous wrapper while retaining unrelated later DSfix edits. It keeps ROMs, generated assets, saves, logs and historical backups.

A former installer setup can be adopted only when its DLL hashes match its recorded ownership. Uninstall then disables MarioSouls instead of restoring an older mod DLL. With no current setup record, uninstall does nothing; manually copied files require [manual recovery](RECOVERY.md).

File-operation checks and successful compilation are not in-game verification. After updating, use a disposable character for the [manual checklist](TESTING.md).
