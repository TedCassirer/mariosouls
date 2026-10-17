# Changelog

Meaningful changes are recorded here by development version. The detailed, maintained inventory of current behavior and tuning is [Features and tweaks](docs/FEATURES_AND_TWEAKS.md). Git retains earlier implementation history; no public release date is implied by this development log.

## 0.9.0-dev.1 - unreleased

### Player installer candidate

- Add a locally built Windows setup executable with game/ROM selection and install/update/uninstall. It shares the existing PTDE setup backend, DSfix handling and recovery rules.
- Embed tracked source and official Python; download hash-pinned tools/packages, build the mod and export maps/equipment on the recipient's computer. Do not bundle ROMs, game exports, prebuilt guest runtime or compiled Mario geometry.
- Preserve source revision/hash provenance in builds from the packaged snapshot. Add a matching source archive, manifest, checksums and draft-release guidance; GitHub Actions is not required.
- Pin Windows Git line endings and use atomic dependency staging/short cache paths. In player asset preparation, record unsupported Havok types as native-only; retain strict source exports by default and flag Ash Lake traversal as unverified.
- The candidate is experimental and unsigned. Installer preparation and compilation do not establish GUI, install/removal or full-game compatibility; publication remains pending review.

### Control fixes

- Remove the legacy Y world-interaction shortcut in Mario mode. A remains contextual jump/interact, Q remains keyboard interaction, and Y still cancels an active cannon sequence.
- Map left-hand Fire Flower casting to LB/L1/right mouse instead of RT/Tab. Keep native spell charges, attunement, reinforcement and damage, right-hand casting, shield LB guard/RT parry and LT/L2 crouch/ground pound.
- Track primary-left and parry presses independently so a held input does not become a different action after an equipment change. Only a shield guard acts as the frontal-critical modifier. Native-character bindings remain unchanged.
- These input changes are compile-checked; live controller, spell-charge and interaction checks remain pending.

### Documentation

- Rewrite the player README around the Mario class, installation, basic controls, native-character toggle and collision overlay.
- Add a detailed feature/tweak inventory with current defaults, implementation references and verification limits. Require its maintenance in AGENTS.md and contribution/PR guidance.
- Document the host/core/SDK/adapter architecture, future Remastered work, capability fallbacks and the distinction between implemented Mario and possible future characters.
- Require fixes developed in forks to be pushed to the fork and proposed to the original project through a PR.

### Source and framework cleanup

- Retain Mario, F6, the initially hidden F8/F9/F10 overlay, DSfix, higher-rate visual interpolation and the technical foundations for a future host port.
- Group gameplay, terrain and drawing entry points, clarify native actor/movement ownership, and separate Mario profiles, pose mapping and item props.
- Remove unused telemetry storage/reads and improve native combat names. Preserve packet validation, native hook instruction sequences/calling conventions and public DLL export names.
- Remove the experimental Wind Waker Link adapter and its exclusive helpers, plus the custom equipment-load stamina penalty. Native armor/ring/stamina behavior remains.
- Remove test labs, executable behavioral suites, command bridges, cheats, recording/replay, F11 reporting, background automation and special Steam-free development-copy tools.
- Keep startup/error logging bounded; remove movement/session recording. Preserve existing local installations, saves, private assets, archives and checkpoint tags.

### Build and local installation

- Keep pinned dependencies/compiler checks, local asset recipes, licenses and build manifests with version, revision, source fingerprints and binary hashes.
- Add compile-only portability coverage for x86/x64 C/C++ SDK/shared declarations and x64 guest components, while retaining the x86 PTDE host.
- Limit CI to dependency preparation, compilation/linking and retained build logs. Build success is not gameplay evidence.
- Replace former installation/packaging tooling with one readable `MarioSouls.ps1 install` / `uninstall` entry point and a PTDE-owned implementation.
- Reuse asset paths/tuning, rebuild stale x86 binaries, preserve the supported DSfix chain/settings, validate ownership/hashes and keep reversible recovery snapshots.

### Gameplay retained from the development baseline

The following features predate the documentation rewrite; they are not newly implemented by it:

- Mario class/creation preview, persistent class intent, manual Mario/native switching, same-instance interactions and one-shot death presentation.
- Native collision/support integration for stairs, doors, dynamic floors, platforms, streamed maps and lethal terrain, with a smaller Mario body and reduced ordinary landing damage.
- Glove-gated Mario attacks, normal Caestus +0..15 progression, Strength/Dexterity scaling, distinct attack windows/stamina, kick push requests and confirmed-hit feedback.
- Supported one-handed weapon families, shields, parries and native critical attempts with authored Mario poses.
- Fire Flower appearance, native Fireball casting with a bouncing projectile profile, lava/fire reaction hooks and bounded hit-recovery presentation.
- Firelink cannon/Bob-omb Buddy, a wider camera, pre-HUD composition, multiple scene/UI resolutions, DSfix integration and visual interpolation.
- A guarded cutscene presentation path using the cinematic camera and player root; it remains a first pass rather than full skeletal retargeting.

See the [feature inventory](docs/FEATURES_AND_TWEAKS.md) for exact tuning and limitations rather than treating this summary as an exhaustive compatibility claim.

### Verification status

Focused historical gameplay checks exist for PTDE with/without DSfix. Recent source cleanup passed compilation and source/file inspection; installation file operations were checked in temporary directories. This documentation rewrite introduces no gameplay change or new live-game evidence. Full-game traversal, cutscenes, hazards, equipment coverage and different machines still need the [manual checklist](docs/TESTING.md).

Remastered, OoT Link and DK64 are not implemented.
