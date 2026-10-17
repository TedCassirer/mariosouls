# Documentation

## For players

Start with the [project README](../README.md), then use:

| Guide | What it answers |
| --- | --- |
| [Installation](INSTALLING.md) | One-command install/update/uninstall, required local data, custom paths and DSfix chaining |
| [Playing](PLAYING.md) | Mario class, controls, gloves, equipment and character switching |
| [Compatibility](COMPATIBILITY.md) | Supported edition, historical stock/DSfix checks and remaining gaps |
| [Troubleshooting](TROUBLESHOOTING.md) | Loading, movement, interactions, rendering, audio and installer problems |
| [Bug reports](BUG_REPORTING.md) | Manual GitHub reports and useful evidence |
| [Features and tweaks](FEATURES_AND_TWEAKS.md) | Detailed behavior, tuning, defaults and source references |
| [Recovery](RECOVERY.md) | Managed uninstall, file backups and source history |

## For contributors

Read [CONTRIBUTING.md](../CONTRIBUTING.md) and [AGENTS.md](../AGENTS.md).

- [Intended architecture and dependency rules](../framework/docs/ARCHITECTURE.md): required contributor/AI-agent design reference, including ownership, failure behavior, change-placement examples and current implementation debt.
- [Development](DEVELOPMENT.md): build dependencies, local assets, outputs and tools.
- [Verification](TESTING.md): compile-only checks and manual game coverage.
- [Release preparation](RELEASING.md): source-based Windows installer and draft-release checks.
- [Framework architecture](../framework/README.md): host, core, SDK and character responsibilities.
- [SDK contract](../framework/docs/CONTRACT.md) and [adapter guide](../framework/docs/ADDING_A_CHARACTER.md).
- [Remastered/host preparation](../framework/docs/HOST_PORTING.md) and [PTDE integration inventory](../framework/hosts/ds1ptde/PORT_CHECKLIST.md).
- [Changelog](../CHANGELOG.md), [license](../LICENSE), [third-party notices](../THIRD_PARTY.md) and [security guidance](../SECURITY.md).

Keep the feature inventory current for behavior/tuning changes and the changelog current for meaningful project changes. Focused guides contain implementation detail; README stays a player introduction.
