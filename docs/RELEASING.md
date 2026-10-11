# Preparing a release

There is **no public release yet**. The commands here prepare local development
archives. They do not create tags, GitHub Releases or Nexus uploads.

## Gates

[Project metadata](../release/project.json) records the development version and
supported executable. [Release checklist](../release/checklist.json) records the
remaining blockers. A pending check must have real evidence before being marked
passed. Shared CI cannot establish live gameplay or clean-PC compatibility.

```powershell
python tools/release.py check
python tools/release.py check --release-ready
```

The first command audits maintained source files and lists blockers. The second
fails while any blocker remains. The scanner rejects common credential formats,
binary content and game-asset extensions; it cannot prove absence of every secret
or settle distribution rights. Review the exact archive manually too.

## Local archives

Commit changes first. Packages use tracked, maintained source, licenses and docs;
historical investigations, saves and game data are excluded. Files are sorted,
ZIP timestamps fixed, and a manifest records every file's SHA-256.

```powershell
python tools/release.py package --kind source --output outputs/MarioSouls-source.zip
python tools/release.py verify outputs/MarioSouls-source.zip
```

An internal candidate additionally includes the x86 host, Mario adapter and SM64
runtime, full-test receipt and build provenance. First run the local build and
ROM-backed tests in [Development](DEVELOPMENT.md), then:

```powershell
python tools/release.py package --kind candidate --output outputs/MarioSouls-candidate.zip
python tools/release.py verify outputs/MarioSouls-candidate.zip
```

Both archives explicitly record `public_release: false` and outstanding blockers.
Candidate packaging refuses stale binaries or changed build sources. A `.sha256`
file accompanies each archive. Existing archives are never overwritten.

These are development bundles, **not drag-and-drop player installations**. The
runtime needs locally supplied assets and prepared game data. No ROM, extracted
collision, equipment mesh or private capture may be added to a public bundle.

## Before eventual publication

Complete the checklist on a clean installation and disposable saves. Validate
[compatibility](COMPATIBILITY.md), install/uninstall, the exact package hashes,
source availability and [third-party notices](../THIRD_PARTY.md). Update the
changelog and known issues. Test the final artifact, not an earlier working tree.

A portable player setup, a local asset preparation flow and source distribution
for bundled libraries still need final design and review. Prepare the public
page and demonstration only after those gates pass. Publishing remains a separate
human decision; no workflow in this repository publishes automatically.
