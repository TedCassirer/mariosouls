# Changelog

## 0.9.0-dev.1 — unreleased

Development build, not a public release. Git preserves earlier source versions;
see [recovery](docs/RECOVERY.md).

### Release preparation

- F11 bug-report review page with prefilled GitHub drafts, bounded structured
  diagnostic history and source build identity; no automatic submission/upload.
- Named shared-test logs and JSON/JUnit results, plus diagnostic regression and
  report-file failure checks. Live game validation remains separate.

- Use Git history for source recovery; retire the separate checkpoint command
  and historical manifests from new source checkouts.
- Keep automatic installation backups, including rollback support for older
  backup locations. Existing local archives and tags are preserved.
- Asset-free Windows CI for framework policies, hook contracts, SDK scaffolding,
  installer transactions and package validation; ROM tests remain local.
- Checksum-verified compiler download and pinned dependency setup.
- GPL-3.0-only license for original code and separate dependency notices.
- Local source and internal candidate archives with inventories and SHA-256 files.
- Unknown loader protection and verified restoration of one install transaction.
- Documented testing, compatibility and explicit public release blockers.

### Existing gameplay baseline

Mario movement and combat, native interactions, equipment, Fire Mario,
class selection, cannon, debugging and transition/collision fixes are present.
These are existing features, not newly verified by the release tooling.
See the [validation record](framework/docs/VALIDATION.md) for actual evidence.

### Known limitations

No standalone player installer. Full-game traversal, scripted platforms,
controller/display combinations and compatibility on another computer still need
validation. Experimental Link content has a separate asset pipeline; OoT Link
and DK64 are not implemented ports.
