# Remastered presentation milestone: implementation handoff

This is a source-level handoff, not a Remastered implementation. PTDE/x86 and Mario/libsm64 are the only implemented host/character. No Remastered installation, addresses, loader, DX11 code, assets or gameplay evidence is supplied. Begin with [ARCHITECTURE.md](ARCHITECTURE.md), [HOST_PORTING.md](HOST_PORTING.md) and the [PTDE integration inventory](../hosts/ds1ptde/PORT_CHECKLIST.md).

This document describes the initial presentation milestone. For later work, the [PTDE behavior and source checklist](REMASTERED_PARITY.md) identifies the tweaks and native integration fixes in the reference source. It leaves implementation order and verification strategy to the port author and does not require restarting this milestone.

## Deliverable and evidence to obtain

Load a separately built x64 host into an owned Remastered installation, validate the native player and camera, and render Mario with correct world occlusion **before** HUD/prompts/text. Native movement, collision, input, HP/stamina, equipment, interaction scripts and combat remain authoritative and active. Native appearance is hidden only for the validated player after a usable guest replacement is available. Invalid observations/draw failures retain or restore that appearance.

Record the actual executable hash/build and PE architecture; loader/export/renderer evidence; actor/camera identity and lifetime evidence; matrix/units/depth conventions; the discovered before-HUD composition boundary; and manual observations of failure/cleanup. Do not infer any hook from PTDE addresses or widen its assembly/layouts. Metadata alone is not an implemented host.

## Reusable entry points already present

| Entry point | Explicit inputs | Output / ownership | Port responsibility |
| --- | --- | --- | --- |
| SDK `SoulsCharacterGetAPI`, `describe`, `initialize` | Exact ABI4/table size, validated descriptor, owned asset/runtime paths and host atlas allocation | API/descriptor borrowed for module lifetime; adapter fills borrowed host atlas | Load matching x64 adapter/runtime, validate version/size/descriptor/assets before callbacks; keep module alive. |
| `core/character_storage.h::MeshBuffers` | Validated descriptor limits | Non-copyable host CPU arrays/atlas and refreshed borrowed `SC_Mesh` | Allocate before borrowing; do not resize/unload while callbacks/rendering/cache use storage. Device reset is not a guest respawn. |
| SDK `create`, `pose`, `destroy` | Semantic native pose, local centimetre position/yaw/phase | One guest instance and validated state/mesh | Use pose-only presentation at a controlled animation cadence. No `step`, movement proposal or outgoing contact in this milestone. Coordinate the single live instance. |
| `core/validation.h::frame` | Callback state/mesh and validated descriptor | Rejection of invalid floats/counts/IDs/buffer shape | Validate every produced frame before it becomes a replacement. A native DLL is trusted code, not sandboxed by these checks. |
| `core/character_presentation.h::camera` | Metre eye/target, FOV radians, aspect, near/far metres; optional validated rigid camera-world matrix | Owned row-major left-handed `SCRenderAPI::Camera`; false on invalid input | Capture actual Remastered camera; establish handedness/depth/raster translation. PTDE lens defaults are edition policy, not portable constants. |
| `core/character_presentation.h::mesh` | Validated SDK mesh/feet in cm, render origin in metres, scale, optional matching history/alpha and rigid model transform | Caller-owned packed metre-space vertices, color/shade/UV | Supply valid stable buffers for the synchronous call. Do not import PTDE actor/cinematic structures; no guest call or native read belongs in this helper. |
| `core/mesh_interpolation.h` | Validated mesh/socket snapshots, explicit time and discontinuity identity | Owned render history/anchor; borrowed sampling output | Preserve existing presentation-only interpolation/snap policy; never use it as native motion or damage state. |
| `sdk/render_api.h::Renderer` | Borrowed meshes/images/camera, explicit material, scene/preview/before-HUD and verified depth availability | Edition-owned GPU resources and synchronous draw results | Implement actual DX11 upload/pipeline/reset/device-loss/state restoration. Do not reuse D3D9 handles, FVF or state blocks. |
| `core/host_capabilities.h` | Verified facility snapshot | Per-operation prerequisites | Publish only evidence-backed actor pose/visibility/depth/before-HUD facilities. Keep movement/combat/input/support facilities unavailable until separately implemented and verified. |
| `core/debug_presentation.h` | Value-only capsules/segments/markers/rings/attack bounds and selected panel labels/counters | Caller-owned bounded line/background/text vertices | Optional after the milestone: supply native observations from this edition and font pixels. Missing observations mean omitted geometry, never guessed bounds. |

These are internal C++ helpers except the explicitly named SDK exports. Do not pass STL/virtual renderer objects across the adapter DLL ABI. Arrays/matrices/strings are borrowed only for the described synchronous calls. CPU images stay immutable and alive until their backend caches are released.

## Build, assets and installation

1. Implement native code in a new edition directory. Do not include PTDE private headers or link its `runtime.cpp`, MinHook-x86 objects or proxy exports into an x64 host. Extract a policy only when it has a concrete neutral need.
2. Add the implemented edition declaration and contained `build(api, output, target)` recipe only with real code/evidence. Select `x86_64-windows-gnu` explicitly, own its exports/hook/backend/link recipe and use the supplied output directory. Shared tools target guest outputs to that edition and inspect DLL PE machine types. Unknown editions fail instead of compiling PTDE.
3. Keep native host/guest/manifest files under `work/framework-build/hosts/<id>/`; PTDE's existing root output remains isolated. Portability outputs under `portability/` are compile artifacts, not installed host binaries.
4. Reuse pinned guest/dependency recipes and user-supplied ROM preparation. Native asset formats/executable fingerprints/configuration and collision/equipment preparation belong to the new edition. A presentation-only milestone does not need guessed native collision assets.
5. Existing `MarioSouls.ps1`, PTDE `setup.py` and the player installer remain PTDE-only. A future installation path needs separate executable, architecture, asset/config and reversible-recovery guards. Do not route it through the PTDE installer or weaken its fingerprint checks.

ABI4 uses `SC_CALL`, fixed-width fields and size/version/layout checks. Native pointer identities remain host-local; x64 archive offsets do not automatically widen. `build --check-portability` checks C/C++ SDK/core declarations for x86/x64 and builds matching guest binaries; no native Remastered binary or behavioral suite is hidden inside that command.

## Lifecycle and activation order

- Observe and validate current actor/root/camera and their generation/lifetime before any presentation work. On invalid identity, stop using its saved buffers/leases and retain native control/appearance. Native pointers must never cross to Mario.
- Load resources independently of the current actor. Keep one explicit instance/presentation owner and separate native visibility restoration identity. PTDE's `AdapterResources`/`Session` describe these lifetimes but contain PTDE/Windows state; reuse neutral storage/policy, not its native fields.
- Build a valid pose-only frame, assemble CPU vertices and successfully establish the actual world composition/depth route. Only then suppress the identified native player's mesh. Suppression/restoration needs a scoped native implementation and live-identity validation.
- Loss of rendering, actor replacement, disable intent, loading or teardown must release the matching visibility lease without restoring stale native fields. Device lifetime and guest/actor lifetime are separate.
- Keep local guards in addition to capability prerequisites: identity, buffer bounds, camera validity, scene recognition and current protected state still matter. Capabilities do not grant locomotion or authorize native writes.

## Scheduling investigation still required

PTDE's `update()` is invoked by graphics callbacks once per Present. Its draw/CPU vertex builders do not advance simulation; its fixed 30-Hz clock, bounded backlog and native acknowledgement are distinct policies. Nevertheless, low render/update opportunity rates can slow the existing guest simulation. The new extraction intentionally leaves that scheduling and every native hook unchanged.

Do not copy Present/EndScene as a universal native scheduler. For Remastered, discover the real update/render threads and ordering, pause/loading cadence, camera snapshot stability, scene/depth lifetime and callback serialization. Pose generation belongs in a controlled presentation/update phase; drawing consumes a stable validated frame. Later locomotion needs evidence for native movement request/acknowledgement and post-resolution observation before guest stepping. Do not change catch-up, guest speed or physics to compensate for an unknown cadence.

## Verification record and next checks

Current preparation compiles PTDE host/Mario/runtime for x86, shared C/C++ declarations and the extracted CPU presentation/storage for x86/x64, and Mario/runtime for x64. It also inspects x64 guest/runtime PE machine types. `doctor` checks dependency/profile/selected-boundary patterns. No check program or behavior test is executed. The C ABI and PTDE hook signatures/addresses/scheduling remain unchanged.

Fresh **manual PTDE verification is pending** for this extraction. Use a disposable character, preserve the level-65 safeguard and obtain a current control handoff before agent input. With stock rendering and DSfix, check:

- Load/F6, walking/turning/jumps, existing 60-FPS interpolation and camera framing.
- Creation preview, native-owned actions/menus, Estus/door/NPC orientation and resuming control.
- Glove/weapon/cannon/impact presentation, attacks and incoming hurt/death/respawn.
- Seamless zone/actor replacement and moving support without stale appearance or restoration.
- F8 hidden initially, F9 layers/F10 Xray, panel labels, terrain/contact retention.
- Resolution changes/device Reset, world occlusion and prompts/subtitles/HUD, including cinematic camera/root presentation.

For the future Remastered milestone, record native walking/turning, wall occlusion, text/menu ordering, actor replacement/loading, resolutions/reset and failure fallback on the owned installation. Keep movement/collision/interactions/combat explicitly unimplemented until their later milestone evidence exists. No installation or publication was part of this source preparation.
