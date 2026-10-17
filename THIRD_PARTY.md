# License, dependencies and assets

Original MarioSouls/Souls Character Framework code is **GPL-3.0-only**; see [LICENSE](LICENSE). This covers original framework/tool/prototype work except attributed upstream material. Contributions to original code use the same license. Third-party material retains its own notices.

## Dependency notices

| Dependency | Use | Notice |
| --- | --- | --- |
| [libsm64](https://github.com/libsm64/libsm64) | Locally built guest runtime with project patches | [CC0](licenses/libsm64-CC0.txt) |
| [SM64 decompilation](https://github.com/n64decomp/sm64) | Pinned cannon/runtime source preparation | [CC0](licenses/sm64-CC0.txt) |
| [MinHook](https://github.com/TsudaKageyu/minhook) | Native hooks | [License and disassembler notices](licenses/MinHook.txt) |
| [Zig](https://ziglang.org/) | Local pinned build compiler | Keep the upstream license with the local tool archive; compiler is not a bundled runtime dependency |
| [Soulstruct](https://github.com/Grimrukh/soulstruct) | Local PTDE asset extraction | Downloaded at pinned revisions on the recipient's machine; not embedded in the setup executable |
| [Python](https://www.python.org/) | Windows setup runtime | Official embeddable runtime; original `LICENSE.txt` remains in the embedded archive and cache |
| [Git for Windows / MinGit](https://github.com/git-for-windows/git) | Installer source retrieval | Downloaded and hash-verified; retain notices in the downloaded distribution |

Build revisions/compiler hashes and runtime geometry inputs are pinned in [dependencies.lock.json](dependencies.lock.json). License copies correspond to those dependency checkouts. Review notices when changing dependencies. Python asset/formatting packages and GitHub Actions are build infrastructure, not bundled game runtime libraries.

## Owned content and future distribution

ROMs, original game binaries, saves, extracted models/textures/audio, native collision and equipment exports are not included in the source repository. Players supply their own local game data. The project's GPL does not grant rights to Nintendo or FromSoftware content.

The guest build compiles upstream model geometry. Its geometry inputs are separately pinned; the older revision used by libsm64 lacks its own `LICENSE.md`. The supplied notices identify dependency sources and do not independently clear all compiled content for distribution.

Player distributions need review of their exact source and binary contents. The source-based setup executable embeds our tracked source and official Python; it downloads the pinned dependencies and builds the guest runtime locally instead of bundling compiled guest geometry or private game exports. Installer downloads are pinned in [release/installer.lock.json](release/installer.lock.json). These notices are not a conclusion that every possible package is permissible. Keep original license texts unchanged and private assets outside Git.

Development has been assisted by **OpenAI Codex**. Generated promotional art, music and footage are not part of this source project.
