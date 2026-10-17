# Preparing a Remastered or other host port

Remastered was considered when separating the framework, but **only PTDE is implemented**. There are no verified Remastered hooks, native layouts, DX11 renderer or installer. Actual work requires an owned installation and fresh native evidence.

## Reusable foundations

| Foundation | What exists | What still requires a new host |
| --- | --- | --- |
| Character SDK | C linkage, explicit `SC_CALL`, version/size gates, fixed-width values and borrowed buffers | Correct target ABI and loading/lifetime integration |
| Shared core | Lifecycle, motion/contact policies, timing and capability requirements | Native observation, ownership and safe control handoffs |
| Rendering | Mesh/texture/camera/depth/phase interface | DX11 resource/device/reset/composition implementation |
| Timing | 30-Hz accumulator, bounded backlog, native readiness/backpressure, visual interpolation | Appropriate native update/acknowledgement hooks and frame-rate verification |
| Assets/config/install | PTDE-owned recipes and reversible setup behind one public command | Edition-specific formats, executable validation, paths and loader arrangement |
| Portability | x86/x64 header/ABI and x64 guest compilation | A running 64-bit native host and gameplay evidence |

Portable currently means the checked Windows x86/x64 components. It is not demonstrated Linux/macOS support. On-disk 32-bit fields remain format values rather than pointers to widen blindly.

## Native isolation

Keep addresses, structures, calling conventions, animation/equipment IDs, map logic, native health/stamina/damage and extraction in the host edition. Core/SDK use neutral semantics and opaque identities; guest engine IDs and ROM/model formats stay in adapters.

The [PTDE integration inventory](../hosts/ds1ptde/PORT_CHECKLIST.md) records each boundary's purpose, inputs, outputs and verification method. Treat its addresses as PTDE evidence, never as offsets to mechanically adjust for Remastered. Recheck registers, stack cleanup, x87/SSE/MXCSR state, pointer ownership and hook failure paths.

## Rendering and timing obligations

`sdk/render_api.h` is an internal C++ interface, not the adapter DLL ABI. It carries row-major world/view/projection matrices, borrowed vertices/RGBA textures, depth availability and submission phase. A new renderer must explicitly translate its API conventions and discover the real scene/UI boundary. Missing depth/before-HUD support suppresses normal world composition rather than drawing Mario over text.

Do not couple simulation to Present calls. Preserve native observation/update, fixed guest steps and visual interpolation as separate operations. The current PTDE implementation permits one guest tick per update, caps backlog and clears paused/invalid ownership intent. A future change to catch-up policy needs explicit gameplay review and corresponding documentation, not an incidental FPS adjustment.

## Host capability requirements

| Operation | Required verified facilities |
| --- | --- |
| Movement | Motion + health |
| Combat | Health + stamina + native damage |
| Interaction | Native interaction routing |
| Platforms | Motion + dynamic support |
| World drawing | Scene depth + before-HUD composition |

These are host facilities, distinct from guest `SC_CAP_*` flags. An unavailable facility must leave a predictable native fallback or disable the operation. It must not grant free attacks, fabricate a floor, alter quests or assume protection from an animation.

## Port checklist

1. Obtain/fingerprint the executable and identify renderer, loaders, archives and save/config boundaries.
2. Create a separate edition host; rediscover each required native integration using the PTDE inventory as a list of responsibilities.
3. Compile and inspect layouts/calling conventions; establish hook/pointer/signature validation before enabling writes.
4. Implement native motion/support, authoritative HP/stamina/damage, interactions and actor/map lifecycle.
5. Implement the graphics backend, camera/depth, scene/UI recognition and device/resource lifetime.
6. Prepare edition-specific assets/configuration and reversible installation. PTDE exports do not establish another edition's compatibility.
7. Use disposable characters to verify loading, death/respawn, streaming, replacement, platforms, native control, combat and rendering at varying rates.
8. Record exact build/game evidence and only then revise support claims in README/compatibility and the feature inventory.

Never test on the owner's level-65 character. No stub host, simulated success or x64 compile should be described as a working Remastered port.
