# Preparing another host edition

**PTDE is the only implemented host. Remastered has no native bindings, DX11 backend or installer in this repository.** An owned installation and fresh native evidence are required. x64 guest compilation is preparation, not a functioning port.

Start from the responsibilities in the [PTDE integration inventory](../hosts/ds1ptde/PORT_CHECKLIST.md). Do not translate its addresses, IDs, layouts, formats or register hooks mechanically.

Follow [ARCHITECTURE.md](ARCHITECTURE.md) for dependency direction, authority, phase/lifetime rules and explicitly bounded current exceptions. This guide supplies the port sequence/evidence obligations; it does not authorize bypassing those boundaries or adding a new abstract host system before an actual implementation needs it.

## First Remastered milestone: presentation with native movement

Keep native locomotion, collision, input, HP/stamina, inventory and combat in control. The first deliverable is a separately loaded x64 host displaying Mario at the validated native actor using the correct camera, depth and before-HUD boundary. No guest movement takeover or attack hooks are required for this milestone.

1. Record the owned executable's hash/build, PE architecture, loader arrangement and renderer. Establish reversible edition-specific loading and a refusal path before native writes.
2. Validate player identity/lifetime, root position/facing and camera matrices against the running installation. Invalid observations retain the human and native controls; rendering must not dereference stale actors.
3. Load the x64 adapter and guest runtime using existing ABI/size/calling-convention checks. Produce presentation poses without stepping locomotion or submitting contacts. Preserve single-instance guest ownership.
4. Implement the graphics backend and resource lifetime. Discover actual scene depth and the composition point before native HUD/text. Do not guess projection/composition or use Present as a simulation clock.
5. Hide only the identified native player draw after a usable replacement exists. Restore native visibility on failure, replacement, disabled intent or teardown. Do not hide NPCs or a global character category.
6. Manually observe native walking/turning, wall occlusion, prompts/subtitles/menus, loading/actor replacement, resolutions and graphics reset/recreation. Record exact game/mod builds and gaps.

This is a milestone definition, not verified Remastered behavior. A presentation-only bootstrap does not establish movement, interaction or combat support.

For concrete reusable entry points, ownership, missing-operation fallbacks and acceptance evidence, use [REMASTERED_HANDOFF.md](REMASTERED_HANDOFF.md). It includes the scheduling investigation still required; no actual Remastered host is supplied.

## Rendering boundary

[render_api.h](../sdk/render_api.h) is an internal C++ interface, not the character DLL ABI. It describes existing presentation needs:

| Submission | Inputs / meaning |
| --- | --- |
| Camera | Row-major world/view/projection; host-supplied conventions |
| Triangle mesh | Metre-space vertices, packed AARRGGBB color/shade and UV; borrowed unindexed triangle list |
| Material/texture | Immutable RGBA8, vertex/modulate/texture-alpha color, optional shade, point/linear sampling, depth test/write, alpha cutoff and alpha blending |
| Lines | Metre-space colored segments; depth-test or through-walls mode |
| Screen triangles | Target-pixel position, reciprocal homogeneous W, color and UV; backend translates raster/clip conventions |
| Context | Scene, before-HUD or preview phase, and actual depth availability |
| Viewport | Target-pixel rectangle for screen-layout scaling |

Character vertices/camera matrices come from [core/character_presentation.h](../core/character_presentation.h), using explicit validated mesh/actor/lens inputs. CPU prop/cannon transforms, screen quads and fitting live in [core/render_geometry.h](../core/render_geometry.h). [core/debug_presentation.h](../core/debug_presentation.h) expands bounded value-only shape observations through [core/debug_geometry.h](../core/debug_geometry.h) and assembles diagnostic panels. Edition code selects native equipment/sockets, captures camera/actor/debug observations and translates native state into labels/counters; it does not own reusable vertex/text assembly. Equipment props, cannon, hit flashes and F8 diagnostics use the same renderer as Mario. Their presentation code contains no D3D9 drawing/resource calls.

PTDE's [render_device.h](../hosts/ds1ptde/render_device.h) owns FVF, pipeline state, RGBA upload and cached managed textures. CPU images remain immutable/alive for the host session; Reset clears backend textures for lazy re-upload. [font_atlas.h](../hosts/ds1ptde/font_atlas.h) rasterizes a Windows font to RGBA without a graphics device. GDI is not a portable renderer requirement.

Camera extraction/lens defaults, D3D9 state capture/restoration, scene/depth surfaces, DSfix sizing, preview targets, cinematic recognition and final composition remain PTDE-owned. Matrix construction itself is neutral CPU code; PTDE supplies validated camera/actor inputs through `render_observation.h`. GPU restoration is a backend-owned synchronous `StateScope`. This is not a generic render graph. A DX11 implementation must translate conventions/resources and rediscover its scene/UI boundary. Before-HUD submissions refuse missing depth; diagnostic through-walls drawing disables depth testing only within a valid composition.

## Selecting and building a host

The shared CLI resolves the selected edition's build recipe instead of linking PTDE unconditionally:

~~~powershell
python tools/framework.py doctor --host ds1ptde
python tools/framework.py build --host ds1ptde --check-portability
~~~

The default remains PTDE. An absent/unimplemented edition fails before compilation. There is no Remastered switch secretly building PTDE and no generated host scaffold.

A real new implementation belongs in its own directory under **framework/hosts/**. Its **edition.json** declares matching ID, implemented status, compiler target, executable fingerprint, executable/config/default paths, asset recipes, build_recipe and build_output. The recipe exports **build(api, output, target)** and owns native sources, hook-library objects, exports, link libraries and target checks. Use the supplied output parent for generated headers/objects and explicitly pass the selected compiler target. Declaring metadata alone never establishes support; native validation and milestone evidence must exist.

**tools/host_editions.py** validates identifiers, implementation status, target, contained recipe paths and the executable fingerprint declaration. PTDE's recipe refuses non-x86 targets. The shared builder targets adapter/runtime to the selected edition and inspects DLL machine types before recording its manifest. Portability compilation checks x86/x64 C/C++ declarations and x64 guests without executing check programs.

PTDE keeps its existing **work/framework-build/** output paths. Other editions use **work/framework-build/hosts/<id>/**, including separate adapter/runtime/manifest files, so x64 work cannot replace PTDE outputs. Manifests record the selected host/target, revision and hashes.

Asset commands dispatch the selected edition's collision/equipment recipe and default game path; **--game** overrides that path. Formats and validation remain edition-owned. The player installer and PowerShell setup stay PTDE-only and retain their host/architecture/executable guards. Build selection is not installation support: implement separately validated, reversible setup for another edition only when it can actually be installed.

## Capability activation and native fallback

Host facilities differ from guest SC_CAP_* flags. [core/host_capabilities.h](../core/host_capabilities.h) defines requirements; PTDE [capabilities.h](../hosts/ds1ptde/capabilities.h) translates verified hooks and current readable observations into one snapshot. Local readiness flags remain underlying evidence/diagnostics, not parallel support declarations.

| Operation | Required facilities | Unavailable behavior |
| --- | --- | --- |
| Presentation | Validated actor pose/camera | No replacement from invalid observations |
| Native replacement | Actor pose/camera + scoped visibility | Retain human visibility |
| Movement | Motion + authoritative health + input routing | Refuse/release guest locomotion; native control remains; presentation is independent |
| Combat | Health + stamina + native damage | No outgoing guest damage/stamina charge or unpaid attack window |
| Interaction | Native interaction routing | No unverified input pulse; native menus/ownership remain authoritative |
| Platforms | Motion + dynamic support | No unverified platform contact or invented floor |
| World drawing | Scene depth + identified before-HUD phase | No world replacement over native UI |

PTDE uses common checks at movement activation/continuation, presentation, action eligibility, combat/equipment/object contacts, native platform observation and before-HUD composition. Its native damage transaction also installs the stamina filter, so their readiness evidence is currently coupled. Actor health/stamina/pose availability uses current validated reads. Before-HUD availability is scoped to the identified draw boundary, not a permanent startup flag.

Capabilities are prerequisites, not permission by themselves. Ownership/generations, menus/paired/death state, individual signatures, configuration, target handles, asset bounds and guest opt-ins still have to pass at each operation. Preserve low-level refusal guards; never mark a missing hook ready to progress.

## Later integration sequence

After presentation-only work, port and verify each responsibility against the owned installation:

1. Native cadence, movement request/acknowledgement, collision/contact/support, actor/map replacement and moving platforms. Native resolved motion remains authoritative.
2. Input and interaction ownership: prompts, menus, Estus, ladders and doors; resume guest poses at the native result.
3. Authoritative health/stamina/status/death, attack packets/defenses/target identity, equipment and spells. No direct guest HP damage or jump invulnerability.
4. Class intent/creation preview, cinematics, environment/fire/projectiles, feedback and optional cannon integration.

Keep native observation/update, fixed guest simulation and rendering separate. PTDE consumes at most one 30-Hz guest tick per update with bounded backlog/native acknowledgement. Preserve pause/ownership clearing and visual interpolation; a different catch-up policy is a gameplay change.

Record purpose, exact inputs/outputs, native evidence, ownership/failure path and manual verification for each integration. Update contract/inventory when boundaries change. This preparation introduces no behavioral suite, simulated-host runner or gameplay automation. Use disposable characters and the [manual checklist](../../docs/TESTING.md); never test on the owner's level-65 character.
