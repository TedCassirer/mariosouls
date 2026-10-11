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
New gameplay changes belong in `framework/`; `work/` is a private local workspace.
Keep host integration, shared policy and character behavior in their respective
directories. Update the SDK and adapter guide when the interface changes.

Run checks appropriate to the change. For gameplay changes, use the supported
build/test workflow and distinguish offline results from live-game results.
Document unverified behavior and keep private test captures out of Git.

Install `requirements-dev.txt` in your development environment. Run
`python tools/framework.py format` before committing; CI checks the same styles
with `format --check`. Use descriptive names and comments that explain ownership,
native ABI constraints and non-obvious decisions. Avoid narrating simple code.
Keep developer tests in `tools/tests/` and gameplay policy tests in
`framework/tests/`; do not add one-off scripts or generated output to source.

Add live regression scenarios to the registry in `framework/qa/__main__.py`.
They must expose help and remain safe to import without native game-access
dependencies. Parse and validate arguments before importing those dependencies;
keep the existing test-character and focus guards in the scenario itself.
The [QA guide](framework/qa/README.md) explains the fixtures and command.

Do not add game assets, dependency checkouts, generated caches, binaries, secrets
or personal saves. The `.gitignore` uses an allowlist; new source directories
need an explicit entry. Preserve existing checkpoint tags and local archives.

Open a focused pull request explaining the problem, changed behavior, validation
and remaining limitations. A development branch is not a tested player release.
