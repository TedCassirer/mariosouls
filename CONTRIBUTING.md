# Contributing

MarioSouls is still experimental. Small fixes with clear reproduction steps and
honest validation are especially useful.

## Report a bug

Open a [GitHub issue](https://github.com/TedCassirer/mariosouls/issues) with:

- The commit/build and Dark Souls version.
- Your controller or keyboard/mouse setup, resolution and other installed mods.
- The area, steps to reproduce, expected result and actual result.
- Relevant log excerpts or a short screenshot/video if useful.

Use the [debug overlay](framework/docs/DEBUG_VIEW.md) for collision problems.
Do not upload ROMs, saves, extracted game files or full memory dumps. Review logs
for personal paths before sharing them.

## Change the code

Start with [development setup](docs/DEVELOPMENT.md) and [AGENTS.md](AGENTS.md).
New gameplay changes belong in `framework/`; `work/` is historical material.
Keep host integration, shared policy and character behavior in their respective
directories. Update the SDK and adapter guide when the interface changes.

Run checks appropriate to the change. For gameplay changes, use the supported
build/test workflow and distinguish offline results from live-game results.
Document unverified behavior and keep private test captures out of Git.

Do not add game assets, dependency checkouts, generated caches, binaries, secrets
or personal saves. The `.gitignore` uses an allowlist; new source directories
need an explicit entry. Preserve existing checkpoint tags and local archives.

Open a focused pull request explaining the problem, changed behavior, validation
and remaining limitations. A development branch is not a tested player release.
