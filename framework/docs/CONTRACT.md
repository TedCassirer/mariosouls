# Character SDK contract

This document describes the current **ABI 4** and independently versioned extensions. The declarations in [character_api.h](../sdk/character_api.h), [class_api.h](../sdk/class_api.h), [launcher_api.h](../sdk/launcher_api.h) and [environment_api.h](../sdk/environment_api.h) are authoritative. Native addresses/layouts belong in the [PTDE integration inventory](../hosts/ds1ptde/PORT_CHECKLIST.md), not in an adapter contract.

PTDE/Mario is the implemented integration. The same boundaries are intended for other characters and a future Remastered host, but neither a compiled scaffold nor x64 guest compilation establishes a working port.

The intended placement/dependency/authority rules are in [ARCHITECTURE.md](ARCHITECTURE.md). This contract defines the exact data and callback boundary; neither document makes another edition or guest engine implemented. Keep adapter C ABI, internal renderer C++ interface and native hook conventions separate when extending them.

## Binary boundary

Export `SoulsCharacterGetAPI` with C linkage and `SC_CALL`. Return null unless the requested ABI version and `SC_CharacterAPI` size match. ABI 1..3 adapters are not accepted by this host. Optional exports also require an exact independent version/size handshake.

Host and adapter must have matching architecture. PTDE uses `x86-windows-gnu`; portability compilation also checks `x86_64-windows-gnu`. Use fixed-width scalar fields and the declared calling convention. Do not pass STL, exceptions, allocator ownership or native game pointers across DLLs. SDK validation is not a sandbox for an untrusted native DLL.

Descriptors/profile data must remain valid while the DLL is loaded. Frame buffers and callback arguments are borrowed; do not retain their pointers after the call or free host allocations. Do not block on gameplay I/O or throw through a callback. Current update/simulation/render-facing callbacks are serialized by the host; audio is not a separate concurrent guest callback thread. Hot adapter unloading/swapping is not implemented.

## Ownership

| Owner | Responsibility |
| --- | --- |
| Native host | Authoritative HP, stamina, inventory/equipment, effects, quests, damage/defenses, death and rewards |
| Shared policies | Session/lifecycle, timing, validation, bounded motion/contact rules and capability requirements |
| Guest adapter | Engine input/actions, movement proposal, mesh/animation, contact eligibility and character-specific presentation |
| Renderer | Consume validated snapshots using edition-owned camera/depth/UI integration |

Adapters must not read/write native memory. Independent guest health/death must not debit native HP or trigger guest warps. Ordinary jumping and guest action flags never imply native invulnerability. The host must avoid duplicate damage when guest/native contact describes the same event.

## Units and data

- Positions, surfaces, distances and attachment translations: **local centimetres**, Y up, native world X/Z axes.
- Velocity/forward speed: centimetres per second. Angles: radians, yaw zero along +Z and positive toward +X.
- `moveX`: right; `moveY`: forward relative to the supplied camera direction.
- Guest simulation: 30 Hz. An engine with another native rate must translate/resample within its adapter.
- `rawAction`, `rawFlags` and other raw diagnostic fields are opaque. Host behavior must use semantic SDK flags/IDs, not interpret guest action numbers.

The host converts native metre-space coordinates and rebases a local origin. `setPosition` supports that rebase; it is not permission to bypass native collision. On-disk indices/offsets remain file-format fields rather than native pointers to widen for x64.

`SC_Mesh` is an unindexed triangle list. Position/normal/color arrays hold `capacityTriangles * 9` floats, UV `capacityTriangles * 6`. Every successful output sets `numTrianglesUsed` within capacity and the descriptor limit. Current maxima are 16,384 triangles, 32 attack descriptors and a 4096 x 4096 RGBA8 atlas. Non-finite coordinates, invalid attack IDs and out-of-bounds counts are rejected.

The base character uses one atlas plus vertex colors. General multi-material rigs are not provided by renaming a descriptor; a shared extension would be needed. Native equipment props are rendered separately at guest attachment transforms.

## Lifecycle callbacks

| Callback | Contract |
| --- | --- |
| `describe` | Stable, validated descriptor with compiled identity, capabilities, attacks, body and texture/audio bounds |
| `initialize` | Resolve owned asset/runtime data and fill the host atlas; failed initialization must tolerate `shutdown` |
| `shutdown` | Release adapter/runtime resources safely, including partial initialization |
| `create` / `destroy` | Return a nonnegative guest handle or -1; allow a later recreate without retaining stale per-instance state |
| `step` | Advance one fixed guest simulation tick and return semantic state plus bounded mesh |
| `pose` | Present a native-driven semantic action on the instance without independent locomotion/combat |
| `resume` | Restore locomotion at host position/facing and clear stale momentum |
| `setPosition` | Apply origin rebasing without inventing a new simulation step |
| `command` | Apply only the declared semantic idle/fall command |

Mario's runtime allows one live instance. The host coordinates class preview, normal presentation, locomotion, death and cutscene lifetimes instead of assuming multiple simultaneous guest engines. Pose-only creation must work without a loaded static floor; spawn/creation/cinematic presentation is not permission to run attacks or native movement.

Session intent is separate from ownership. A saved Mario class may request appearance while native spawn/menu/recovery state prevents locomotion. Actor/physics/movement replacement invalidates old bindings and queued work. A seamless map change with the same valid native identity refreshes terrain/motion state without needlessly destroying the guest or losing paid contact windows.

Host-owned visibility is conditional on a usable replacement mesh. A failed/ambiguous draw must retain native appearance rather than hide the only player. Explicit toggle-off, teardown and safe lifecycle handbacks restore owned native state.

## Motion reconciliation and terrain

`resolveMotion` receives `SC_MotionResolution`: corrected local position, ownership generation, movement sequence and up to four unit contact normals. Zero sequence can describe an unsolicited correction. Apply the correction to state and existing mesh together. Remove only momentum into supplied normals; preserve tangential velocity, current action and animation phase.

This callback is not a physics/animation tick. It must not consume or re-emit attack/landing events, invent ground or reset all velocity. Empty normals mean a position correction without cancelling momentum.

The PTDE bridge publishes one bounded movement request, consumes it in the native movement hook and samples native position before permitting the next guest tick. This is acknowledgement of consumption, not a separately hooked Havok post-solve callback. Native support/contact observations remain authoritative for event floors, doors, actors and platforms.

`surfaces(..., dynamicContacts=0)` replaces static guest geometry. `dynamicContacts=1` replaces a short-lived native contact set of at most 48 triangles; zero count clears it. Surface material is currently reserved/zero. Actor capsule tangents must not be fabricated into floors, and a removed event floor must not persist in static geometry.

`SC_CAP_NATIVE_WALLS` permits the host to omit near-vertical cached walls while retaining floors/ceilings, only when verified native collision resolves motion. Native live contacts can still support wall actions. Absence of a guest-cache triangle is neither a verified wall nor a verified floor; the guest must allow native resolution to reach event-controlled geometry.

## Simulation and presentation timing

The shared accumulator is independent of rendering and caps backlog. PTDE currently consumes at most one 30-Hz tick per update, subject to native acknowledgement; low update rates can still slow simulation. Focus loss, invalid ownership and protected native actions clear queued intent rather than replaying a long stall.

Visual interpolation operates on validated mesh/attachment and native-anchor history at render cadence. It adds about one guest tick of visual delay without changing input, native collision, hit points, audio, stamina or damage. First frames, actor/ownership changes, gaps, teleports and incompatible topology/material order snap rather than blend. Rendering never calls `step`, `pose` or `resolveMotion`.

## Input, poses and native actions

`SC_CAN_INTERACT` describes guest eligibility; `SC_GROUNDED` describes actual support. The host still checks native prompts, footing, control locks and action state. In PTDE, controller A is routed once to an eligible interaction or to jump. Menu entry/exit and focus/controller/actor changes cannot reuse that press. Keyboard Q requests interaction, Space jump and X/E selected-item use. Y only cancels an active cannon sequence; it never requests world interaction.

Native scripts perform talk/use/rest/climb/door operations. The adapter receives a semantic `SC_Pose`: neutral, climb, drink, rest, door, walk, downed, talk, stagger, death, native combat or opt-in knockdown. Phase -1 is free-running; other phases are normalized 0..1. Climb travel is signed cumulative vertical centimetres. Unsupported presentation should use a stable fallback rather than invent an interaction result.

Native takeover suspends guest locomotion/contacts. `resume` continues the same guest at the resolved native location with stale momentum cleared. Native menus, paired attacks, grabs and death retain control. Bounded native footing/idle preparation occurs before an input pulse; animation alone does not establish safe control return.

`SC_CAP_ITEM_PROP` means the adapter supplies its own held use-item prop, allowing the host to suppress the native human-hand prop. `SC_CAP_DEATH_POSE` opts into a one-shot guest death pose with no attack/interaction/attachment output. Native HP/death/respawn stay authoritative; a held final mesh must not delay native progression.

Cutscene rendering is host-owned presentation, using native cinematic transforms and protected control. PTDE's neutral-pose replacement is not skeletal retargeting. Invalid/ambiguous player parts retain native rendering.

## Attack windows and native damage

`character.json` generates a compiled descriptor; editing an installed JSON file alone does not retune that DLL. Attack index/name pairs must distinguish the first punch, second punch and finishing kick. Report -1 outside an attack. Repeated instances of an identical ID need an intervening idle sample; there is no independent guest attack-instance token in ABI4.

`SC_ATTACK_ACTIVE` marks the contact phase. `activeTicks` is an additional upper bound, not permission to damage throughout an animation. The host owns serials, paid/exhausted state, target deduplication, range/height/occlusion and maximum targets.

Charge native stamina once per intended damaging attack. An unpaid/exhausted move has no damaging window. Navigation cannot acquire an added combat charge just because its animation is airborne. Automatic stomps use a separate eligibility/airborne-use guard; an exhausted or repeated contact cannot create unlimited bounce/damage loops.

`probeContact` is read-only. It checks guest action/facing against a host-selected point on the target collision volume, which can be above the feet on a large target. It must not mutate attack/health/action state or bounce Mario. `commitContact` applies an accepted guest-side contact effect only after host acceptance. Native damage is not implemented by subtracting target HP in the adapter.

Power is pre-defense native attack power. Host damage packets preserve native defenses, blocks, poise, deaths and rewards. Weapon attacks use their native equipment path; guest attacks must not silently inherit unrelated weapon power/status effects. Actor generations/handles are host-private and must be revalidated after streaming/recycling.

## Equipped items and attachments

`SC_CAP_EQUIPMENT` enables neutral `SC_EquipmentPose` and `SC_Attachments`. The host owns native inventory/action selection, stamina, guard and damage; equipment callbacks present those decisions without stepping locomotion.

`SC_CAP_MELEE_FAMILIES` extends the ordinary sword/shield presentation with dagger, curved sword, greatsword, axe and hammer semantics. `SC_CAP_NATIVE_COMBAT` adds backstab/riposte/parry presentation. `SC_CAP_PYROMANCY` enables right/left cast semantics. Do not send unsupported kinds/actions to an adapter that has not opted in.

PTDE maps LB/L1/right mouse to the left primary action: native cast for a Flame, guard for a shield. RT/Tab requests shield parry; LT/L2 remains guest crouch/ground pound. Primary-left and parry edges are tracked independently so item changes cannot turn one held button into another action. These bindings are host input policy, not guest action IDs or a change to the binary SDK.

Attachments are column-major rigid transforms with an orthonormal basis and local-cm translation. +Z follows the blade/shield front and +Y is up; valid bits identify right/left hands. They must describe the same mesh frame returned by the current step/pose/correction, not an older independently sampled skeleton. Native props use those sockets; rendered interpolation cannot change authoritative hit sockets.

Native parry/critical/cast work owns movement until native completion. The guest has no simultaneous unarmed contact window during a protected native action. Unsupported families/two-handing/critical opportunities retain defined native or disabled behavior rather than fabricated success.

## Audio

`SC_CAP_AUDIO` supplies interleaved stereo PCM through `audio`. Queue, desired, capacity and returned counts are **sample frames, not bytes**. Never exceed capacity or reinterpret native audio ownership. Mario audio is an additional channel; host enemy/music systems remain native.

## Capability and optional-export reference

Adapter flags are declared in `character_api.h`:

| Flag | Meaning |
| --- | --- |
| `SC_CAP_AUDIO` | Guest PCM audio |
| `SC_CAP_ITEM_PROP` | Guest use-item prop presentation |
| `SC_CAP_TEXTURE_MODULATE` | Multiply atlas RGB by vertex RGB |
| `SC_CAP_ALPHA_TEST` | Discard atlas alpha <=127 and use texture alpha |
| `SC_CAP_EQUIPMENT` | Equipment pose and attachment callbacks |
| `SC_CAP_DEATH_POSE` | One-shot guest death presentation |
| `SC_CAP_MELEE_FAMILIES` | Extended neutral melee kinds |
| `SC_CAP_NATIVE_COMBAT` | Native parry/critical pose semantics |
| `SC_CAP_PYROMANCY` | Native caster pose semantics |
| `SC_CAP_NATIVE_WALLS` | Verified native wall authority |

Unknown capability bits are not a fallback promise to older hosts: validation can reject them. Missing rendering opt-ins retain the original texture/color behavior.

The following exports use independent version 1 profiles/APIs and do not change ABI4 layouts:

| Export | Purpose | Missing/invalid behavior |
| --- | --- | --- |
| `SoulsCharacterGetCameraProfile` | Bounded distance scale | Keep native camera unchanged |
| `SoulsCharacterGetProjectileProfile` | Supported native projectile/cast tuning | Keep native projectile behavior |
| `SoulsCharacterGetImpactProfile` | Attack-specific native push requests/cooldown | No added guest push profile |
| `SoulsCharacterGetReactionProfile` | Native poise/ordinary reaction tuning | Preserve the existing native/fallback reaction path |
| `SoulsCharacterGetRecoveryProfile` | One-shot flinch/knockdown presentation and safe completion policy | Do not assume normalized shortened recovery |
| `SoulsCharacterGetUnarmedEquipmentProfile` | Semantic standard-fist gate, saved upgrade range and text | Preserve legacy independent guest-attack behavior |
| `SoulsCharacterGetClassProfile` | Class text/archetype and preview framing | No class replacement from that profile |
| `SoulsCharacterGetLauncherAPI` | Guest launcher state and world-prop presentation | No guest launcher objects |
| `SoulsCharacterGetEnvironmentAPI` | Native floor/fire observations to guest reactions | Native damage remains, without this guest reaction extension |

Profiles are bounded by shared validation. A semantic standard-fist slot is mapped to native inventory by the host; adapters do not contain the native item ID. A class archetype is likewise mapped by the edition host rather than exposing a native class record.

## Launcher and environment extensions

Launcher `begin` requires a consumed eligible world interaction. Guest `step` advances opening/aim/flight; `query`, `texture` and `mesh` are observations. `cancel` is idempotent. The host owns placement, prompt priority, line-of-sight, input arbitration, native motion and camera submission. Native prompts win. Camera leases are actor/camera-bound and expire if not renewed. Flight uses ordinary native collision/health; the API does not supply a complete imported NPC/object engine.

Environment observations use semantic burning-floor/enemy-fire kinds plus contact/protection/ground/control flags. They occur before simulation/control reconciliation and do not advance time. The guest can request a reaction and priority over ordinary hurt presentation, but cannot alter native HP, stamina, inventory, protection or damage multipliers. Death, grabs and protected scripts still win. Details and verification gaps: [environment reactions](ENVIRONMENT_REACTIONS.md).

## Internal renderer and host capabilities

[render_api.h](../sdk/render_api.h) is an internal C++ boundary, not a cross-DLL character ABI. It takes borrowed RGBA textures and vertices, **row-major** world/view/projection matrices, explicit scene/before-HUD/preview phase and depth availability. No D3D handles cross it. Another renderer must translate conventions/resources explicitly.

The same interface submits character/equipment/cannon triangles, colored diagnostic lines, impact flashes and screen text/reticles. `Material` makes color/texture mode, filtering, shade, depth test/write, alpha cutoff and blending explicit. Screen coordinates are target pixels with reciprocal homogeneous W; colors are AARRGGBB. CPU images are immutable and must outlive the backend cache (PTDE retains them for the host session); submitted vertices are borrowed only for the call. Renderer resources/state remain backend-owned. `core/render_geometry.h` provides CPU prop/cannon/quad helpers, without native or graphics calls. This internal interface change does not change ABI4.

Host facilities in `core/host_capabilities.h` are separate from adapter flags. Presentation requires current actor pose/camera; replacement also requires scoped native visibility. Movement requires verified motion/health/input routing; combat health/stamina/native damage; interaction native routing; platform support native motion/support; world drawing scene depth/before-HUD. PTDE translates its hook flags/current validated reads in `capabilities.h` and uses these operation checks at activation and feature entry points. Capabilities do not replace identity/menu/death/signature checks. Missing facilities disable the operation or retain native behavior, without invented collision/damage; presentation remains independent of guest movement readiness. Details: [host porting](HOST_PORTING.md).

PTDE's D3D9/DSfix backend is implemented. A Remastered/DX11 backend and native bindings are future work; x64 header/guest compilation does not implement them.

## Evolving the contract

Preserve C/C++ layout and calling-convention assertions. Change the main ABI when its binary layout/required semantics change; use separately versioned, exact-size optional profiles where appropriate. Update this contract, the [adapter guide](ADDING_A_CHARACTER.md), [host-porting guide](HOST_PORTING.md), [feature inventory](../../docs/FEATURES_AND_TWEAKS.md) and [changelog](../../CHANGELOG.md) with resulting behavior and verification limits.
