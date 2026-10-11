# MarioSouls

Super Mario 64 movement and combat inside **Dark Souls: Prepare to Die Edition**.

Run, jump, punch and ground-pound through Lordran, while Dark Souls handles enemy
health, stamina, equipment, progression and rewards. MarioSouls runs on the Souls
Character Framework, which separates each character from the game's integration.

**Status: experimental development build.** There is no standalone installer or
public download yet. This repository contains source code, not a playable package.
Dark Souls Remastered is not supported.

## What works today

- Mario's movement and unarmed moveset, with controller and keyboard/mouse input.
- Native interactions, including bonfires, items and bloodstain recovery.
- Six melee weapon families, shields, parries and critical attacks.
- Fire Mario appearance and bouncing fireballs when using a Pyromancy Flame.
- A Mario starting class, death animation, wider camera and collision diagnostics.
- An experimental SM64 cannon and pink Bob-omb Buddy in Firelink Shrine.

These features have different levels of testing. Area transitions, collision and
scripted interactions are still being refined; this is not a claim of full-game
compatibility. See the [validation record](framework/docs/VALIDATION.md).

## Getting started

| You want to… | Start here |
| --- | --- |
| Play the mod | [Player guide and release status](docs/PLAYING.md) |
| Build or work on it | [Development setup](docs/DEVELOPMENT.md) |
| Understand the framework | [Architecture and documentation](framework/README.md) |
| Add another character | [Adapter guide](framework/docs/ADDING_A_CHARACTER.md) |
| Report a problem or contribute | [Contributing](CONTRIBUTING.md) |
| Understand tests and CI | [Testing](docs/TESTING.md) |
| Prepare a future release | [Release process](docs/RELEASING.md) |
| Recover an earlier version | [Checkpoints and local backups](docs/CHECKPOINTS.md) |

Mario requires a user-supplied US Super Mario 64 ROM. ROMs, saves, extracted assets,
generated collision data and binaries are excluded from Git. A portable installer
that prepares the required data locally is planned.

## Project layout

```text
framework/   Active game integration, character adapters, SDK and tests
tools/       Build, validation, dependency and checkpoint commands
docs/        Player, development and recovery guides
patches/     Changes applied to pinned upstream dependencies
checkpoints/ Historical manifests; runtime archives stay local
work/        Preserved prototype, investigations and local build files
outputs/     Historical local controls, installer and restore helpers
```

New gameplay work belongs in `framework/`. The scripts in `work/` and `outputs/`
include historical and machine-specific workflows; they are not a public installer.

## Credits

Built using [libsm64](https://github.com/libsm64/libsm64),
[MinHook](https://github.com/TsudaKageyu/minhook),
[SM64 decompilation](https://github.com/n64decomp/sm64) and the
[Soulstruct tooling](https://github.com/Grimrukh/soulstruct).
Development has been assisted by OpenAI Codex. Dependencies retain their own
licenses; third-party game assets are not included.

Original project code is [GPL-3.0-only](LICENSE). See [third-party notices](THIRD_PARTY.md)
and the [unreleased changelog](CHANGELOG.md).
