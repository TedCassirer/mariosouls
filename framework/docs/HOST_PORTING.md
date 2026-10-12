# Host edition preparation

**PTDE is the only implemented host. Dark Souls Remastered is not supported.**
No Remastered executable, addresses, hooks, renderer or compatibility tests are
provided. Owning and inspecting that installation is required before a port can
be implemented. Passing the portable suite is not evidence of Remastered support.

## Boundaries and audit

The October 2026 audit found the gameplay address tables, native structures,
map identifiers, equipment IDs, animation IDs and packet construction already in
`hosts/ds1ptde/`. Native QA scripts were outside that boundary; they now live in
`hosts/ds1ptde/qa/`. Installation, restoration, configuration rewriting and
executable validation now live in the same host. The tools directory retains
compatibility imports so existing automation uses the same implementation.
The executable fingerprint moved from project metadata to the host edition
manifest. Diagnostic journals now receive their host identity from the caller.

`core/` holds policies, not native observations. `sdk/` carries neutral units and
semantic equipment/action categories. Character adapters own guest action IDs,
asset offsets, runtime loaders, movement, animation and props. N64 segmented
addresses in Mario's asset decoder are guest-format data, not PTDE addresses.
Windows DLL loading in adapters is an operating-system dependency; portable here
means Windows x86/x64, not demonstrated Linux/macOS compatibility. Guest runtime
binary formats may use 32-bit offsets regardless of the process pointer width.

The architecture check rejects graphics/Windows/host includes in shared code,
checks adapter headers as well as source for native memory/hook APIs, and keeps
opaque guest action fields out of host policy. This is a guardrail, not a complete
static analysis of arbitrary C++. Review new constants and casts for provenance.

## Rendering

`sdk/render_api.h` is an internal C++ interface, separate from the character DLL
ABI. It submits borrowed meshes, RGBA8 textures and explicit camera matrices, with
scene/preview/before-HUD phase and depth availability. PTDE's `render_device.h`
implements it and the production character draw path uses it for texture upload,
camera transforms and triangle submission. D3D types and GPU resource ownership
remain inside the host. Camera matrices are row-major, with host-defined projection
conventions; a different graphics backend must translate them deliberately.

PTDE still owns lighting, pipeline state, native camera reads, the managed atlas,
render target/depth capture, composition, UI recognition, preview target, props,
effects and device reset. This preparation does not pretend those game-specific
passes are portable. A future backend must implement the submission interface
and its own scene/depth/HUD integration. No DX11 backend has been created.
Missing depth or a recognized UI boundary suppresses normal before-HUD composition;
legacy explicit render modes retain their existing behavior. Synthetic D3D9 tests
cover RGBA conversion, matrices, multiple scene/UI resolutions, depth occlusion,
UI priority, state restoration, failed-draw retry and reset.

## Architecture and ABI

ABI4 remains unchanged. `SC_CALL` documents the existing C calling convention.
DLLs must match their host's architecture; x86 and x64 tables are not wire formats
and must never be mixed. Pointer-free SDK payloads have fixed sizes on both targets.
Pointer-bearing tables have explicit per-target size/offset assertions. A C program
also includes and exercises the public headers. The PTDE composition root rejects
any build where pointers are not 32 bits.

The asset-free suite executes the shared policy tests on both Windows x86 and
x64, compiles Mario and Wind Waker Link adapters on both, and loads an unrelated
generated adapter through each architecture's DLL ABI. This verifies compilation
and ABI behavior; real owned-asset runtime validation remains a separate test.
`build_runtime.py --target x86_64-windows-gnu` is a guest-runtime validation recipe,
not a host build or installer. With owned assets, run `python tools/framework.py
test --portable-assets --character mario64 --asset <owned-rom> --runtime <x64-sm64.dll>`.
For Link use `--character link_ww --asset <private-scmesh>` with no runtime.
These checks never authorize installation. Production `build` selects the x86 PTDE host.

## Timing and lifecycle

`core/timing.h` owns the fixed 30 Hz accumulator, independently of graphics and
native updates. `advance` accepts elapsed simulation time, `take` consumes only
when native motion is ready, and `clear` discards time on ownership/pause changes.
Backlog is capped at two ticks; backpressure retains at most one. A caller may
drain bounded catch-up; **PTDE retains its existing one-tick-per-update caller**.
Consequently very low PTDE update rates still slow guest simulation. Changing that
requires separate gameplay verification and is deliberately outside this change.

Tests cover 15/24/30/60/144/240 Hz input cadence, repeated draws without elapsed
time, pauses, invalid deltas, long stalls, catch-up and native acknowledgements.
They do not assert unverified arbitrary-rate gameplay equivalence.

`simulated_host_test.cpp` is a fixture around production `Session`, transition
classification, class selection, motion mailbox, support inference, clock and
input latch. The actual PTDE lifecycle uses the same transition classification.
The fixture verifies loading, delayed safe activation, seamless maps without rig
replacement, invalidation on actor/physics replacement, death and same-pointer
respawn, temporary native control, moving/removed platform observations, clock
wrap, activation cooldown and the test-character guard. It does not reproduce a
physics engine, save format, native animations or enemy damage implementation.

## Host capabilities

`core/host_capabilities.h` distinguishes host facilities from guest `SC_CAP_*`.
Capabilities describe observed availability, not a promise derived from edition
name. The normal UI composition path uses the depth/before-HUD requirements.
The simulated host tests every missing bit independently. Existing PTDE movement,
combat and interaction checks remain the authority for those live operations;
the enum does not grant them new functionality or bypass signature validation.

| Operation | Required facilities | Unavailable behavior |
| --- | --- | --- |
| Guest movement | Native motion and health observation | Do not activate simulated movement; keep native control |
| Guest combat | Native health, stamina and damage | Do not manufacture damage or free attacks |
| Interaction | Native interaction routing | Leave native routing; do not emulate quest effects |
| Platform support | Native motion and dynamic support | No guessed floors without confirmed native support |
| Normal world draw | Captured scene depth and before-HUD boundary | Skip composition; never cover the HUD |

A future host should publish only verified facilities and route operations through
these requirements. Native HP, stamina, inventory, quests and rewards remain the
single authority. Jumping never grants implicit native invulnerability.

## Edition selection and installation

`tools/host_editions.py` only permits `ds1ptde`. Native build, doctor, test,
install, uninstall and host asset recipes accept `--host ds1ptde`; unknown editions
fail before building or modifying files. There is intentionally no Remastered
entry with invented defaults. Character asset preparation stays adapter-owned;
host collision/equipment extraction is edition-owned. Keep private caches tied
to the exact executable hash. Never reuse PTDE collision caches, equipment props,
configuration or installation receipts as proof of another edition's compatibility.

The existing transactional backup/rollback, exact-build receipts, closed-game
check and save/test-character protections are preserved. No live save or installed
game is changed by these offline preparations.

## Future port acceptance checklist

1. Obtain an owned installation; fingerprint executable/architecture and inspect
   renderer, data formats and mod-loader interactions.
2. Implement a separate host directory and edition manifest. Re-discover every
   integration below; neither address offsets nor native IDs are portable facts.
3. Implement native observation/motion/health/stamina/action boundaries with exact
   structure and calling-convention tests. Validate signatures fail closed.
4. Implement graphics submission, depth/resource/reset handling and a verified
   before-HUD boundary. Exercise synthetic renderer checks before live checks.
5. Run x64 core/SDK/adapter tests, owned-asset adapter tests and host simulations.
6. Test disposable guarded characters: load, death/respawn, streaming while
   moving/attacking, moving/removable platforms, interactions and enemy defenses.
7. Verify edition-specific asset prep, reversible install and uninstall, and
   unsupported-executable rejection. Keep support claims absent until evidenced.

See the [PTDE integration checklist](../hosts/ds1ptde/PORT_CHECKLIST.md).
