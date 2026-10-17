# Release preparation

GitHub Releases do not require Actions. Build locally, then upload the files to a draft prerelease. Publish only when the human explicitly requests a release; mark development versions as prereleases and describe remaining verification. Successful compilation does not establish a stable player release.

## Build the installer

Run `python tools/build_installer.py` on Windows from a clean, committed checkout. It uses .NET Framework's C# compiler and writes the setup executable, matching source ZIP, `installer-manifest.json` and `SHA256SUMS.txt` under `outputs/release/<version>/`.

The executable embeds tracked project source and checksum-verified official Python 3.13.16 for Windows x64, including Python's original `LICENSE.txt`. It does not embed mod/game DLLs, compiled Mario geometry, the guest runtime, ROMs, collision/equipment exports, saves, logs, credentials or local dependency trees.

On the recipient's computer, PTDE's `player_setup.py` downloads pinned MinGit/Python wheels and existing compiler/source dependencies, builds the mod, exports 17 maps and supported held props from the chosen game, and calls the existing `setup.py`. The recipient needs internet on first install and Windows 10/11 x64/.NET Framework 4.8. The game host remains PTDE/x86. Existing DSfix detection, refusal checks, backup/rollback and uninstall rules stay shared with the PowerShell entry point.

`release/installer.lock.json` pins Python/MinGit/wheel URLs, sizes and hashes and the Soulstruct source revision. `python tools/build_installer.py --prepare-lock` deliberately refreshes these pins; review and commit the result before packaging. Downloaded content stays private. The ordinary compile-only CI is unchanged.

Building the guest runtime on the recipient's computer avoids bundling its compiled upstream geometry here. It does not grant rights to Nintendo/FromSoftware content. Review [THIRD_PARTY.md](../THIRD_PARTY.md) and retain notices.

## Candidate verification

The player exporter explicitly leaves unsupported Havok shape types in native collision and records each omission/reason in its asset manifest. It does not generate replacement geometry. Source CLI exports remain strict by default; `--native-fallback` opts in. The current parser does not understand some Ash Lake convex shapes, so their guest movement/support still needs a location-specific game check before making traversal claims.

- Compile PTDE/portable components with `python tools/framework.py build --check-portability`; packaging compiles the C# front end.
- `Setup.exe --inspect <private-output.json>` checks embedded payload hashes and exports package metadata, without installation or game control.
- In a fresh private cache, run the embedded Python with `player_setup.py prepare --game <PTDE DATA> --rom <owned ROM>`. It builds and exports assets without changing game files.
- Inspect filenames/content, licenses, hashes and private-data exclusions. Attach source and checksums with the executable.
- Before claiming a stable player release, manually verify GUI install/update/uninstall on a disposable game copy, with/without DSfix, interrupted setup, protected folders and a second PC. An explicitly requested experimental prerelease must clearly list any of these checks that remain pending. Then follow the [manual game checklist](TESTING.md). Never use the owner's level-65 character.

Report compilation, payload inspection, preparation, installation/removal and live gameplay separately. The first candidate is experimental; pending checks are not implied to pass.

### Evidence for the first candidate (2026-10-18)

- C# compilation, embedded-payload hash inspection and PTDE/x86 plus x86/x64 portable compilation passed.
- The bundled Python environment downloaded pinned tools/packages, built the host/adapter/runtime and exported all 17 map caches, 82 rigid props and 574 equipment rows. One Ash Lake collider (`h9200B2`) remained native-only because its Havok type is unsupported.
- The shared setup backend installed/removed files in minimal disposable PTDE directories in direct and DSfix modes. A repeated DSfix update reported already current; uninstall restored DSfix's exact original bytes and kept its 60-FPS settings. These were file-operation checks, not game launches or GUI click-through checks.
- Payload inspection found no ROMs, game exports, prebuilt mod/guest binaries or credential-pattern matches. The embedded Python license was present. The existing real installation's managed hashes stayed unchanged.
- GUI click-through, protected folders, a second machine and fresh gameplay/Ash Lake support checks remain pending. No new live-game compatibility is claimed.

## Draft prerelease

Push reviewed source, select a new version tag at its commit, and create a draft prerelease. Attach the `.exe`, matching source ZIP, manifest and SHA-256 list. Include owned-ROM requirements, first-run downloads/building, supported PTDE edition, DSfix handling, unsigned status, controls, known limitations and actual verification. Never reuse checkpoint tags or silently replace an existing asset.

Draft preparation alone is not approval to publish or upload to Nexus. An explicit request for a GitHub release authorizes that release; it does not change repository visibility, authorize Nexus uploads or authorize installation into the maintainer's game. Keep the source revision/tag and all asset checksums matched. Create a new version instead of replacing another draft's assets. [GitHub's release guide](https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository) covers manual uploads; Actions minutes are not required.

## 0.9.0-dev.2 preview

This preview adds the host presentation/state/debug boundary extraction and a candidate Fire Flower cast-handoff fix. Left/right casts settle native footing before input dispatch; the reported left-hand failure has not been reproduced and verified on this revision. Describe it as a candidate, not a confirmed gameplay fix. The previous installer's preparation/file-operation evidence remains historical. Current compilation and package inspection are separate from pending GUI/second-PC/live-game checks.
