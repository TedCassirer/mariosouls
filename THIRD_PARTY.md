# License and third-party notices

Original MarioSouls and Souls Character Framework code is licensed under
**GNU GPL version 3 only**. See [LICENSE](LICENSE). This applies to original code
in the framework, tools and preserved prototype, except material attributed to
upstream projects. Contributions use the same license.

Third-party material retains its original license. This repository's GPL does
not grant rights to Nintendo or FromSoftware game content.

| Dependency | Use | Upstream notice |
| --- | --- | --- |
| [libsm64](https://github.com/libsm64/libsm64) | Mario runtime, locally built with our patches | [CC0 notice](licenses/libsm64-CC0.txt) |
| [SM64 decompilation](https://github.com/n64decomp/sm64) | Cannon/runtime source preparation | [CC0 notice](licenses/sm64-CC0.txt) |
| [MinHook](https://github.com/TsudaKageyu/minhook) | Native hooks | [License, including disassembler notices](licenses/MinHook.txt) |
| [Zig](https://ziglang.org/) | Downloaded build tool; excluded from packages | Upstream license stays in the local tool archive |
| [Soulstruct](https://github.com/Grimrukh/soulstruct) | Optional local game-data preparation | Not bundled; retain upstream notices if distributing it later |

Pinned build revisions and compiler checksums are in
[dependencies.lock.json](dependencies.lock.json). License copies correspond to
those local dependency checkouts. Review notices again whenever dependencies
change. Python CI dependencies and GitHub Actions are build infrastructure,
not bundled runtime libraries.

ROMs, extracted models, textures, audio, collision caches, saves and original
game binaries are not included. Players must supply their own game data locally.
The guest runtime compiles upstream model geometry. Its geometry inputs are
separately pinned in the lock file; the older revision used by libsm64 does not
include its own LICENSE.md. The supplied notices identify the dependency
checkouts, and do not clear that compiled content for distribution.

A future distribution still needs review of its exact source and binary contents;
these notices are not a conclusion that every distribution is permissible.

Development has been assisted by **OpenAI Codex**. No generated promotional art,
music or footage is included in the source or development packages.
