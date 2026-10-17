## Problem and resulting behavior

Describe the concrete problem, reproduction and what this change makes the player or developer observe. Link the related issue if available.

## Architecture and ownership

For framework changes, identify the owning host/core/SDK/adapter/tool responsibility and authoritative state. Describe changed dependencies, inputs/outputs, units/lifetimes/actor identity, capabilities/local eligibility and unavailable-feature behavior. Mention timing/render and ABI implications, or state that the boundary is unchanged.

Check the [architecture rules](../framework/docs/ARCHITECTURE.md): no native/graphics dependencies in core/SDK, no guest internals/character-name gameplay branches in hosts, no native game access in adapters and no gameplay work in drawing. Distinguish intended design from remaining implementation debt; explain any affected bounded exception instead of silently expanding it.

For presentation/session changes, identify the snapshot producer, CPU buffer owner, borrow duration, current-actor validation and native restoration authority. Keep native observations separate from shared character/debug builders; report remaining edition-local state/include-order debt.

## Validation

List compilation actually run and its result. Record manual game checks separately, including build/settings/area, and identify unverified behavior. A build or scaffold is not gameplay/port evidence.

## Documentation

- Update `docs/FEATURES_AND_TWEAKS.md` for behavior, tuning, default or configuration changes.
- Add meaningful changes to the current unreleased `CHANGELOG.md` entry.
- Update player instructions, SDK/adapter contract or host-porting inventory where affected.
- Update `framework/docs/ARCHITECTURE.md` and AGENTS/contributor guidance when responsibilities, dependency rules or the known-debt list change.

## Compatibility and delivery

Mention SDK/layout, migration, native hook, asset, installation or DSfix implications. Keep Remastered/other-character claims consistent with what is implemented.

For a fork contribution, push the fix branch to the fork and target the original repository's appropriate base branch. Reuse the relevant existing PR rather than duplicating it.

Confirm the diff contains no ROMs, saves, game assets, binaries, credentials or private captures. Original project code remains GPL-3.0-only; upstream notices remain separate.
