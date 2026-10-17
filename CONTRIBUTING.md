# Contributing

MarioSouls is a PTDE character mod built on a reusable host/core/adapter framework. Mario is implemented; Remastered and other guest characters are future work. Start with [development](docs/DEVELOPMENT.md), [architecture](framework/README.md) and [AGENTS.md](AGENTS.md).

Read the detailed [architecture rules](framework/docs/ARCHITECTURE.md) before changing framework code or contracts. They distinguish intended boundaries from current PTDE debt. These rules apply equally to human contributors and AI-generated changes; compilation alone does not establish that they were followed.

## Make a focused change

Keep Dark Souls integration in the edition host, neutral policies in core/SDK and guest-engine behavior in the character adapter. Prefer an improvement at the correct framework boundary over a Mario-specific branch in host code. Preserve native health, stamina, inventory and rewards, as well as the existing DSfix integration.

Explain the owning layer/authority and any changed dependency, data lifetime, identity, capability/failure path, timing/render behavior or ABI in your PR. Hosts must not include adapter internals; core/SDK must not acquire native/graphics dependencies; drawing must not advance gameplay. New editions use their own verified implementation/build/install boundaries. Update the architecture/debt record when a boundary actually changes rather than describing the new design as already verified.

Build with:

```powershell
python tools/framework.py doctor
python tools/framework.py build --check-portability
```

CI compiles; there are no automated gameplay suites. Use the [manual checklist](docs/TESTING.md) when game verification is appropriate, and state which cases remain untested. Never use the owner's level-65 character. Agent-driven controls need a current handoff.

Character/camera assembly and debug geometry/panels now have explicit neutral input builders; PTDE supplies observations and owns native restoration/GPU state. Keep their borrowed-input and output lifetimes intact. Use the current resource/session owners rather than introducing a parallel global instance; do not convert native cleanup into destructor side effects. Preview/cinematic/audio state and include order remain bounded debt, documented in the architecture specification. A port starts from the [presentation-only handoff](framework/docs/REMASTERED_HANDOFF.md).

## Keep the behavior record current

Update [features and tweaks](docs/FEATURES_AND_TWEAKS.md) whenever behavior, tuning, configuration or defaults change. Include values/units, source locations, limitations and verification status. Add meaningful changes to the unreleased section of [CHANGELOG.md](CHANGELOG.md).

Update player instructions for visible/control/installation changes, and SDK/porting guides for contract or native integration changes. README is a player introduction; implementation detail belongs in the focused guides.

## Work through your fork

The original project is [TedCassirer/mariosouls](https://github.com/TedCassirer/mariosouls). Confirm the actual fork/upstream remotes and base branch before pushing.

If working in a fork, push fixes to a focused branch in that fork and open a PR to the original project. Reuse an existing PR for the same work. Do not deliver a fork fix only as a local commit or push directly to upstream by mistake.

A PR should describe the concrete problem and resulting behavior, link a reproduction or issue where available, list compilation and manual observations, and identify unverified cases. Include the documentation updates. See the [PR template](.github/PULL_REQUEST_TEMPLATE.md).

## Keep private data private

Commit source, manifests and documentation only. Exclude ROMs, saves, game assets, extracted props/collision, binaries, dependency checkouts, private captures and credentials. Preserve historical tags and local archives. Use [manual bug reports](docs/BUG_REPORTING.md) for ordinary problems and [security guidance](SECURITY.md) for sensitive reports.

Optional formatting uses `requirements-dev.txt` and `python tools/framework.py format`; it is not a CI gate. Contributions to original project code use [GPL-3.0-only](LICENSE); upstream licenses remain separate.
