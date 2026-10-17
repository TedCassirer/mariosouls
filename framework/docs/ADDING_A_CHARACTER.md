# Adding another character

The framework is intended to make another character easier to integrate without repeating Dark Souls hooks. Mario is the only working adapter today. Link, DK or any other guest needs its own engine/behavior/assets implementation; libsm64 does not run those games.

Follow [ARCHITECTURE.md](ARCHITECTURE.md) and its placement/dependency rules. A new character is an independent guest axis, not a host branch or another native memory reader. OS-based loading of its own runtime/assets is allowed; native game observation/actions remain the edition host's responsibility.

## Start at the boundary

Use [character_api.h](../sdk/character_api.h) and the [SDK contract](CONTRACT.md) as the authority. Keep guest APIs, IDs, movement, animation, mesh/prop generation and asset decoding under `characters/<id>/`. Keep native addresses/IDs and state in the host, and reusable policies in core.

```powershell
python tools/framework.py new-character my_character --name "My character"
python tools/framework.py build --character my_character --adapter-only
```

The generated triangle is an explicitly nonfunctional scaffold. Keep that status until real movement, animation and owned-asset loading work. The scaffold does not provide a runtime recipe or a game port.

## Implementation sequence

1. Choose a guest runtime or implement its movement/animation rules and local asset loader. A model swap alone is insufficient. Add optional asset recipes where useful; a separately built guest runtime supplies `build_runtime.py --target <compiler-target> --output <directory>` and must honor the selected host architecture.
2. Export `SoulsCharacterGetAPI` with C linkage and `SC_CALL`. Reject unsupported version/size requests; return only stable descriptors and bounded caller-owned output.
3. Translate neutral input, local centimetres, cm/s and radians. Keep opaque guest action IDs diagnostic-only to the host.
4. Implement safe initialize/shutdown and create/destroy lifetimes, including failed initialization and a later recreate. Mario currently permits one live guest instance; do not assume its runtime is a general multi-instance engine.
5. Implement fixed-step locomotion and `resolveMotion`. Native correction must not advance simulation, animation or attack events, and should remove only velocity into supplied contact normals.
6. Implement same-instance native-driven poses/resume. Pose-only creation must work without static terrain for class previews/spawns; it must not start locomotion, attacks or a second health pool.
7. Describe distinct attack IDs/windows and implement read-only contact probes separately from committed contact effects. Host stamina/damage/target identities remain authoritative.
8. Opt into equipment, death, class, environment, launcher and other capabilities only when supported. Missing profiles retain the contract's fallback rather than pretending the feature works.

## Rendering and equipment

Return stable triangle ordering/material layout when possible so visual interpolation can blend snapshots. Positions/normals/colors/UVs and same-frame attachment matrices must stay within host capacities. Rendering cannot tick the guest.

Character-specific skeletal fitting and prop meshes belong in the adapter. Native equipment selection, damage, guard and action ownership belong in the host. Mario's authored upper-body clips provide one example; they are not a universal rig-retargeting system.

Host presentation submits equipment, cannon, feedback, diagnostic lines and screen geometry through the internal renderer, with explicit material/depth/phase values. Reuse `core/render_geometry.h` for CPU prop/cannon/quad generation; do not add graphics API calls to an adapter. Shared host capabilities gate operations independently: a future presentation-only host may keep native movement while posing the guest. An adapter opt-in never makes an unverified native facility available.

Body scale is distinct from speed and jump distance. `SC_CAP_NATIVE_WALLS` requires verified host motion/collision; it cannot be used to bypass a native wall with guest-only movement. Environment callbacks may request presentation/movement reactions but never write native HP, stamina, inventory or immunity.

The host owns mesh storage (`core/character_storage.h`) and renders validated outputs through `core/character_presentation.h` and the internal renderer. Adapters supply guest meshes/poses; they do not read native cameras or build backend state. Diagnostic observations/panels are neutral presentation data, not adapter permission to inspect the game. Keep all borrowed SDK arrays valid for the documented call lifetime.

## Compile and verify

Build the adapter and full host integration, then use `build --check-portability` for the intended portable declarations/guest outputs. A runtime is built only when the manifest requires one and its recipe exists. No behavioral test discovery or simulated-host runner is available.

Use the [manual checklist](../../docs/TESTING.md) on disposable characters: previews, load/respawn, toggle, transitions, dynamic support, native actions, attacks/stamina, equipment, audio, HUD order and resolutions. F8/F9/F10 expose bounded diagnostics, not exact per-bone native shapes.

Document supported capabilities and remaining gaps in the adapter README and [feature inventory](../../docs/FEATURES_AND_TWEAKS.md). Update SDK/porting guides for shared extensions and [CHANGELOG.md](../../CHANGELOG.md) for meaningful changes. Fork delivery follows [CONTRIBUTING.md](../../CONTRIBUTING.md): push the fix to the fork and create/reuse an upstream PR.
