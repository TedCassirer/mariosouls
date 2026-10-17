# MarioSouls

MarioSouls is a mod for **Dark Souls: Prepare to Die Edition on Windows** that replaces the Pyromancer class with **Mario**. Play the game with Super Mario 64 movement, jumps and attacks, while keeping Dark Souls progression, equipment and world interactions.

Equip Mario's Gloves for punches, kicks, dives and ground pounds.

The mod has been tested in focused development sessions **with and without DSfix 2.4**. It is still experimental. Dark Souls Remastered is a future target and is **not supported yet**.

## Installation

You need your own PTDE installation and a **US Super Mario 64 ROM in big-endian `.z64` format**. ROMs and game assets are not included.

Experimental Windows installer previews are available from [GitHub Releases](https://github.com/TedCassirer/mariosouls/releases). `MarioSouls-<version>-Setup.exe` provides Install/Update and Uninstall, with game-folder and ROM selection. It includes Python, downloads pinned build/asset tools, builds locally and exports data from your own game. No separate developer tools are required. See [installation details](docs/INSTALLING.md) for candidate status and requirements.

To install from a source checkout, first follow the [build and asset preparation guide](docs/DEVELOPMENT.md).

With the game closed, run from the project folder in PowerShell:

```powershell
.\MarioSouls.ps1 install
```

Run the same command to update. The script backs up replaced files, reuses existing asset paths and detects the supported DSfix loader automatically. It preserves your DSfix graphics and frame-rate settings.

To uninstall:

```powershell
.\MarioSouls.ps1 uninstall
```

The default game location is Steam's usual PTDE `DATA` folder. Your ROM can stay anywhere; use `-GameDirectory` and `-Rom` for other paths. See [installation details](docs/INSTALLING.md) for first-time paths, previews and recovery.

## Playing

Start a new game and select **Mario** where Pyromancer normally appears. The tutorial still provides equipment as you progress. Equip **Mario's Gloves** in the active right hand to use Mario's damaging unarmed moves; they reinforce through the normal blacksmith system.

| Action | Controller | Keyboard / mouse |
| --- | --- | --- |
| Move / camera | Left / right stick | WASD / mouse |
| Jump or interact at an eligible prompt | A | Space to jump; Q to interact |
| Attack / right-hand cast | B or RB | Left mouse |
| Left-hand cast / shield guard | LB (L1) | Right mouse |
| Shield parry | RT (R2) | Tab |
| Crouch / airborne ground pound | LT (L2) | Left Ctrl |
| Use selected item | X | E |

Use A for world prompts; Y is no longer an interaction shortcut. Native menus and native-character mode keep their normal controls. More equipment notes are in [the player guide](docs/PLAYING.md).

## Character toggle and collision overlay

| Control | Effect |
| --- | --- |
| F6, or hold both stick buttons | Switch between Mario and the native character |
| F8 | Show or hide the collision/debug overlay |
| F9 | Cycle Combat, Terrain, Objects and All views |
| F10 | Toggle drawing through walls |

The overlay starts off. It shows collision capsules, attack limits, cached terrain and object origins; it is not a complete view of native weapon or per-bone hurtboxes. See [overlay details](framework/docs/DEBUG_VIEW.md).

## How it works

MarioSouls uses [libsm64](https://github.com/libsm64/libsm64) to run Mario's movement and animation. The project builds native DLLs: a Dark Souls host loads the Mario adapter and its libsm64 runtime, connects movement to native collision and draws Mario before the game's interface. Dark Souls remains responsible for health, stamina, inventory, enemy damage, quests and rewards. Mario simulates at 30 Hz, with visual interpolation for higher render rates.

## More information

Use the [documentation index](docs/README.md) for the full guide map.

- [Features and tweaks](docs/FEATURES_AND_TWEAKS.md) and [version history](CHANGELOG.md)
- [Compatibility](docs/COMPATIBILITY.md), [troubleshooting](docs/TROUBLESHOOTING.md) and [bug reports](docs/BUG_REPORTING.md)
- [Framework architecture](framework/README.md), [development](docs/DEVELOPMENT.md) and [contributing](CONTRIBUTING.md)

Built with libsm64, [SM64 decompilation](https://github.com/n64decomp/sm64), [MinHook](https://github.com/TsudaKageyu/minhook) and [Soulstruct](https://github.com/Grimrukh/soulstruct). Development has been assisted by OpenAI Codex.

Original project code is [GPL-3.0-only](LICENSE). See [third-party notices](THIRD_PARTY.md).
