# Build and asset tool reference

`python tools/framework.py --help` lists the shared entry point. Run commands from the repository root. The build CLI does not install, launch or control the game.

| Command | Purpose |
| --- | --- |
| `build [--with-runtime]` | Build the PTDE/x86 host and selected adapter; optionally rebuild the guest runtime |
| `build --adapter-only --character <id>` | Compile only an adapter |
| `build --check-portability` | Full x86 build plus x86/x64 C/C++ declarations and x64 guest compilation; execute no checks |
| `doctor` | Inspect pinned dependencies, architecture boundaries and profile metadata |
| `new-character <id> --name <name>` | Generate an explicitly nonfunctional adapter scaffold |
| `prepare-collision --map <id> --output <fresh-dir>` | Export private edition-owned map data |
| `prepare-equipment --output <fresh-dir>` | Export private native held props |
| `prepare-assets` / `preview-assets` | Dispatch a character's optional recipe |
| `preview-equipment` | Manually render private Mario equipment pose sheets |
| `format [--check]` | Optional local source formatting/checking; not a CI requirement |

`dependencies.py` prepares/checks pinned sources and the compiler. `profiles.py` validates manifests and emits SDK descriptors. `host_editions.py` allows only the implemented PTDE host. `framework_portability.py` performs compilation, not gameplay simulation.

Source install/update/uninstall uses root `MarioSouls.ps1`. The Windows front end reuses `framework/hosts/ds1ptde/setup.py`. `python tools/build_installer.py` packages committed source and official Python locally, without upload/publication. See [release preparation](../docs/RELEASING.md).

There are no test-lab, replay, game-control or developer-session commands. Keep owned assets, dependency checkouts, build outputs and previews private. See [development](../docs/DEVELOPMENT.md), [installation](../docs/INSTALLING.md), [verification](../docs/TESTING.md) and [contribution/documentation rules](../AGENTS.md).
