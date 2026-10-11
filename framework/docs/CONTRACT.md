# Character ABI version 4

Framework 0.4 adds opt-in rendering capabilities without changing ABI structure
layout: `SC_CAP_TEXTURE_MODULATE` multiplies atlas RGB by vertex RGB;
`SC_CAP_ALPHA_TEST` discards atlas alpha <=127 and uses texture alpha in the
character layer. Without these bits, the legacy texture-alpha color blend and
opaque output remain unchanged. Install a matching host; older hosts reject the
new capability bits. All rendering remains before native UI and restores D3D state.

`core/mesh_pack.h` is an optional adapter-side helper, not a mandatory asset format.
SCMESH01 contains an 8-byte magic, five little-endian uint32 fields (texture width,
height, vertex count, bone count, clip count), RGBA8 pixels, 72-byte triangle-soup
vertices, then named clips. Vertex layout: position3, normal3, UV2, RGB3 float32;
four uint16 bone indices; four float32 weights; uint32 visibility group mask.
Each clip has a null-terminated 32-byte name, uint32 frame count and loop flag,
then frame-major/bone-major 3x4 row-major skin matrices at 30 Hz. The reader bounds
size/counts, validates finite values/weights/indices, and rejects trailing data.
Character recipes generate this private pack; the host never parses guest archives.

`sdk/character_api.h` is the authoritative declaration. Adapters are trusted native
32-bit Windows DLLs, loaded in-process; validation is not a memory-safety sandbox.
Export `SoulsCharacterGetAPI` using C linkage and return null for any unsupported
ABI version or API structure size. Functions use the compiler's default C calling
convention. Build for `x86-windows-gnu`. Do not pass C++ containers, exceptions,
allocator ownership or Dark Souls pointers across the boundary.

## Lifecycle and ownership

The host calls `describe`, validates limits, allocates buffers, then calls
`initialize`. The descriptor and its strings must remain valid for the DLL's
lifetime. Initialization fills a single RGBA8 texture atlas. The current host
loads one adapter with one character at a time. `create` returns a nonnegative
handle, or -1. A second live instance should be rejected. Failed initialization
must tolerate `shutdown`; duplicate initialization must fail cleanly.

The host loads static surfaces before creation, steps the character at 30 Hz,
and passes resolved native corrections with `resolveMotion`. `pose` animates the **same**
instance during an interaction; it must not run independent locomotion physics.
`resume` restores locomotion at the host's position and facing. It must clear stale
momentum so drinking or climbing cannot launch the character. `destroy` releases
the instance; a subsequent `create` must work. Buffers remain host-owned. Callbacks
must not retain their input pointers, block on I/O during gameplay, or throw.

Simulation/render callbacks run serially in the host update. There is no concurrent
audio thread. The current process-lifetime host does not hot-unload an adapter.

## Movement ownership (ABI 2)

ABI 1–3 adapters are rejected; rebuild both host and adapter. `resolveMotion` is a
required callback. `SC_MotionResolution` contains the native capsule position in
local centimetres, an ownership generation, a movement sequence (zero for an
unsolicited correction), and up to four unit contact normals. Copy the corrected
position into internal state and output, translate existing mesh vertices by the
same displacement, and remove only velocity directed into the supplied normals.
Do not tick physics, advance animations, invent a floor, or consume/re-emit an
attack/landing event. An empty normal set changes position without cancelling
momentum. `setPosition` remains available for coordinate-origin rebasing only.

The shared host publishes one bounded movement request, the native movement
hook consumes it once, and the next render update samples the native capsule
before permitting another guest tick. This is a consumed-request acknowledgement,
not a separately hooked Havok post-solve callback. Native contacts, combined with
actual position disagreement, identify blocked velocity. Actor capsule tangents
are never turned into guest floor patches. Combat observes the corrected pose.

The guest clock targets 30 Hz with at most one tick per presentation. It does not
replay a stall backlog. Button taps are latched briefly while waiting for native
movement. Focus loss clears queued intent. Requests time out; invalid output or
an unsupported bridge restores native controls. A central session owns the
actor/physics/movement/map binding. Native actions suspend guest locomotion and
resume the same character after 100 ms of stable idle state. Loads, actor changes
and death discard the old instance and all queued work; a requested character
may be recreated only after a living, safe native actor is stable for 500 ms.
An explicit toggle-off cancels that request. Faults require explicit reactivation.

## Coordinates and geometry

Positions, triangle vertices and distances use **local centimetres**, Y upward,
native world X/Z. Velocity and forward speed are centimetres per second. Yaw is
radians: zero points along +Z; positive rotation turns toward +X. Input moveX
is right, moveY forward relative to camLookX/Z. Adapters translate their engine's
units, axes, speed and tick rate here, without changing the shared host.

The host rebases the local origin and resubmits static surfaces when necessary.
`surfaces(..., dynamicContacts=0)` replaces static geometry; 1 replaces up to 48
short-lived native contact triangles (zero clears). Native Havok resolves dynamic
doors, enemies and props. The adapter must respect host corrections, not insist
that a stale guest wall or enemy overlap is authoritative. Surface material is
reserved in v2; the current host supplies zero.

Prepared PTDE collision caches exclude MSB collision parts controlled by the native
event instructions SetMapCollisionState/SetMapCollisionBackreadMaskState. Native
Havok and live contacts own those barriers in both their enabled and disabled
states. The adapter must not recreate a removed barrier. The preparation tool
reads owned game files and refuses unresolved parameterized collision IDs. Maps
without a prepared override retain their legacy cache; they are not implicitly
revalidated by preparing another map.

Attack contact callbacks receive a candidate point on/in the target's native
collision volume, in local centimetres. It is no longer necessarily the actor's
foot origin. Guest callbacks decide action eligibility/facing; they must not assume
a human-sized target. The host checks reach and occlusion to that point, then uses
the same point for native damage and confirmed impact feedback. Automatic downward
attacks use the upper body surface. The host owns target bounds; adapters have no
native pointers or memory access. This clarification keeps ABI 3 unchanged.

Mesh arrays are unindexed triangles. Position/normal/color each have
`capacityTriangles * 9` floats; UV has `capacityTriangles * 6`. Do not write beyond
capacity. Set `numTrianglesUsed` on every successful step/pose. The descriptor
allows at most 16,384 triangles and a 4096×4096 atlas. Frame validation rejects
non-finite output, invalid attack IDs and excess mesh counts and restores native
control. Rendering uses the native camera/depth and composites before Dark Souls UI.
Character meshes use one atlas and vertex colors, without a general material
or skeleton system. Large/multi-material characters require a shared SDK extension.
The optional equipment layer draws separately textured native props through the
same before-UI rendering path; it does not expand the character mesh material API.

## Health and interactions

Dark Souls owns HP, stamina, equipment, inventory, quests, enemy defense, deaths
and rewards. `nativeDamage(handle, severity)` communicates a confirmed host hurt
event (0 normal/no new hit, 1 hurt, 2 strong hurt); it never grants HP or protection.
An adapter must disable independent lethal guest damage and must not add jump
invulnerability. Host damage and guest collision cannot both debit HP for one hit.

`SC_CAN_INTERACT` means the guest's present action can safely start a native
interaction. `SC_GROUNDED` describes actual ground support, not an idle animation.
The host checks native prompts and state as well. Native action scripts perform
the item/door/menu/ladder operation; the adapter receives semantic poses:
neutral, climb, drink, rest, door, walk, downed and talk. A phase of -1 means a
free-running pose; otherwise phase is 0–1. Climb travelCm is cumulative signed
vertical travel. Unsupported optional poses should fall back to a stable neutral
pose, not fail the entire interaction. Talk currently falls back to neutral on Mario.

Controller A is contextual: at an eligible native prompt it requests the native
interaction; otherwise it supplies guest jump. `core/context_action.h` assigns
each press once and consumes interaction presses until release, including across
menu entry/exit. Focus, controller or actor changes require release before a new
press. Native menus retain their normal A confirmation. Y/Q remain explicit
interaction aliases; Space remains jump and X/E remains selected-item use.
The DS1 host recognizes track-14 idle 120000 as well as 100000, so changing
weapon stance does not silently disable interactions. No adapter-specific input
switches or inventory/reward writes are involved.

`SC_CAP_ITEM_PROP` tells the host the adapter draws its own held item during use;
it suppresses the native human-hand prop/effect. Add character-specific fittings
in the adapter, never in host code.

`SC_CAP_DEATH_POSE` opts into `SC_POSE_DEATH`. After authoritative native HP
reaches zero, the host stops guest gameplay, clears pending combat/input, returns
physics ownership to the host, and samples this pose at 30 Hz with `phase=-1`.
The adapter plays its character-specific animation once and holds its final pose;
it must return no interaction/attack flags or attachment sockets. Death visuals
must not run guest death warps, alter either game's rewards, or delay native
respawn. Native identity/map changes, HP returning above zero, invalid transforms
or a 15-second presentation limit release the old guest. The requested character
can reactivate only after the ordinary living/idle stability checks. Unsupported
adapters retain native death presentation. This optional capability extends ABI 4
without changing struct layouts; older hosts reject the new capability bit.

## Attack contract

`character.json` generates the compiled descriptor; editing installed JSON alone
does not change behavior. Attack index/name pairs identify moves. Report -1 when
no attack is underway. Distinct combo punches and finishing kicks need separate
IDs. Repeated instances of the same move must include an intervening idle sample;
the guest attack contract has no explicit attack-instance token. If a future
engine can chain identical attacks without that boundary, extend the SDK and
shared window logic.

`SC_ATTACK_ACTIVE` marks only the actual contact phase. `activeTicks` adds an
upper bound measured from attack start; zero means the semantic phase determines
the bound. `interruptedAttackId` can report a move started and cancelled within the
same guest tick, so it still costs stamina. `landingRecoveryTicks` is an event:
emit it once on landing, not continuously.

The shared window charges native stamina once, rejects damage on exhaustion,
and permits each enemy/object once per individual attack. Exhausted animations
may finish but cannot hurt, stagger or create a bounce. Ordinary movement and
navigation jumps are free. Automatic contact attacks consume an attempt once per
airtime even when exhausted; landing rearms them. `probeContact` must be read-only;
only `commitContact` may trigger the guest's bounce/hit response after a paid hit.

Guest/unarmed attack power is independent of equipped weapons. Shared scaling
uses base + upgradeStep × level + strengthBonus + dexterityWeight × strengthBonus(dex).
The bonus awards 3/stat over 10 to 20, 2/stat over 20 to 40, then 0.5/stat to 99.
The native host inserts attack power before native defenses. Range, vertical bounds,
target count, multiplier, stamina cost and poise are manifest-driven; shared stagger
limits and equipment-weight costs apply downstream. The current host supplies
physical melee/area contact attacks. Equipped sword attacks use the separate native
weapon path described below. Projectiles, magic and target-lock
abilities need new shared capabilities, not hidden character-specific hooks.

`rawAction`, `rawFlags` and related fields are opaque diagnostics only. No host
gameplay branch may inspect engine IDs. The architecture check catches direct
runtime imports and common forms of this dependency, but code review is still needed.

## Audio

With `SC_CAP_AUDIO`, `audio` writes interleaved signed 16-bit stereo at audioRate.
Lengths are sample **frames**, two int16 values per frame. Never exceed
capacityFrames, even if desiredFrames is larger. Return zero for insufficient
capacity or absent audio. Without the capability, provide a no-op callback.


## Body dimensions and landing damage (ABI 3)

`SC_Descriptor.body` declares model scale, native movement radius/total height,
secondary contact radius/total height (centimetres), and landing damage policy.
Zero for all four dimensions preserves the host body. Default model scale and
fall multiplier/cap are 1; default falls may kill. PTDE currently supports only
smaller human capsules (radius 10-40cm, height up to150cm; secondary radius up to45cm,
height up to160cm). Larger bodies require additional host support.

The renderer scales vertices around the character's feet, including held props;
this does not change movement velocity, jumps, or manifest-defined guest attack
reach. Equipped blade contacts follow the scaled rendered prop. Adapters
must also adapt their own wall/ceiling queries to the declared body dimensions.
Mario's runtime builder applies bounded source transformations to a generated
copy of the pinned dependency; it never edits the preserved dependency checkout.

PTDE validates actor ownership and capsule shape types, retains the original
fields, and changes sizes at the native movement hook before Havok. The body stays
small during native interactions. Releasing the character queues restoration at
the next native movement update; stale actor/physics references are discarded.

Fall policy scales only the framework's landing damage and caps it as a fraction
of maximum native HP. `nonlethalFalls` also caps at current HP minus1 and removes
the mid-air lethal-height check. It does not enable NoDamage/NoDead flags, heal
the player, modify enemy damage or exempt native death volumes. The baseline safe
and lethal distances still define the unscaled curve. Mario defaults: visual0.9,
movement44cm wide/105cm tall, secondary48cm wide/110cm tall, multiplier0.05,
maximum5% HP per landing, and nonlethal falls. Live checks confirm smaller native
capsules, reduced actor separation and retained melee damage. This is not a
per-bone hitbox rewrite; projectile, grab and boss-specific hit routes need separate
coverage. See VALIDATION.md for measured scope.

## Read-only diagnostics (ABI 3 unchanged)

The host may call `probeContact` repeatedly to visualize eligible points without
advancing a frame. It must be a read-only predicate: no animation changes, attack
window changes, bounce, damage, random-state consumption, or contact side effects.
Only `commitContact` may apply an accepted contact reaction. The shared diagnostic
overlay uses descriptor ranges, current state and this predicate for every adapter;
it contains no character-ID branches. See DEBUG_VIEW.md for view semantics and limits.

## Equipped melee and guard (ABI 4)

PTDE 0.7 supports one-handed straight swords, daggers, curved swords, greatswords,
axes, and hammers/maces, plus left-hand small/medium shields. This is two ground
light attacks and held guard, plus optional native parry/critical presentation.
Native heavy attacks, two-handed moves and a complete animation port remain out
of scope. Flexible Gargoyle Tail Axe
model 509 is excluded. Other weapon families and temporary weapon-buff parity
remain outside the verified scope. Mario supplies the first fitted adapter.

`SC_CAP_EQUIPMENT` opts into two required callbacks: `equipment` accepts a
host-owned presentation snapshot and `attachments` returns hand/socket matrices
from the same rendered mesh frame. Neither callback advances simulation. The
capability is optional; adapters without it provide null callbacks. All adapters
must rebuild for ABI4. `SC_Input.guard` is a held input, never buffered as a press.

`SC_CAN_EQUIP_ACTION` advertises safe grounded locomotion states in which a host
may begin equipped attacks or guarding. The adapter must exclude pain, burning,
intangible, cutscene, climbing and special attack states. This is distinct from
interaction eligibility. The host additionally requires native state, stamina,
ownership and supported gear; the flag grants no damage protection.

`SC_CAP_MELEE_FAMILIES` (64) additionally opts into right-hand dagger (3), curved
sword (4), greatsword (5), axe (6), and hammer (7). It requires `SC_CAP_EQUIPMENT`.
Existing equipment-only adapters receive only sword (1) and shield (2); none is 0.
This extension preserves ABI4 layouts and old enum values.

`SC_EquipmentPose` has right/left kinds, swing 0/1/2, normalized
phase and native-confirmed guard 0..1. The adapter fits arm poses to its skeleton
while its ordinary engine retains locomotion. No forced idle ticks or extra
animation advancement. Unsupported states may invalidate the sockets.

`SC_Attachments` contains column-major rigid matrices. Translation is in local
centimetres; basis vectors must be finite, unit and orthogonal. The +Z axis points
along the sword blade or shield front. Asset-specific fitting belongs to the
adapter, with no native pointers. The host converts canonical metre-sized props
through the matrix and applies the descriptor's model scale. It uses the same
blade endpoints for presentation and swept contact. `resolveMotion` and
`setPosition` shift sockets with the mesh. Destroy/resume/native interactions and
damage clear pending poses and sockets; stale matrices must never survive a new
character or origin.

PTDE owns equipment selection, supported item family, native behavior lookup,
one stamina charge per swing, per-target attack windows, native weapon packet,
guard input filtering and actual guard observation. Empty right hand still uses
the adapter's independent attack profile. The equipped ID is resolved to its
native parameter row and reinforcement level, retaining its upgrade path. Native
behavior selects attack data and stamina cost; the native damage routine builds
weapon power and scaling before enemy defenses. The host checks the resulting
packet's selected weapon and hand, and stops equipped damage if validation fails.
Item stats, defenses, elemental powers, native status and rewards are not replaced
by direct HP writes. Data-path support is not live validation of every upgrade,
elemental/status effect or temporary buff.

Authored family timings at 30 Hz (total ticks, inclusive contact ticks):

| Family | Total | Contact |
| --- | ---: | --- |
| Dagger | 18 | 4..9 |
| Curved sword | 22 | 5..11 |
| Straight sword | 24 | 5..12 |
| Axe | 32 | 8..16 |
| Hammer/mace | 34 | 9..17 |
| Greatsword | 36 | 9..18 |

Pose phase maps windup end to .20, contact end to .55, and recovery end to 1.
These are authored guest timings, not claims of exact native TAE frame parity.

Late recovery accepts one new press; holding attack cannot repeat forever. Gear
change, damage, ownership loss or ineligible state cancels without refund. No
stamina means no swing. Guarding routes only the native guard button, slows guest
ground movement, and displays a raised shield only after native guard confirms.
Jump/crouch/attack/interaction release guard. Blocking never refunds HP or sets
NoDamage; native guard stamina, shield defenses and guard breaks remain native.

Axe/hammer contact uses the distal 45% of the prop with a bounded head radius
from its transverse bounds (9..22 cm); cutting weapons use blade-axis contact.
Contact sweeps between consecutive valid hand poses against native actor
capsules, checks occlusion, and accepts each target once per swing. Contacts use
the same canonical blade endpoints as rendering; they do not use a generic
Mario punch range. Warps, stale sockets and unsupported gear cancel the work.
The shield mesh is presentation only: native guard state and native hit resolution
decide protection, not overlap against the displayed polygon surface. Props may
temporarily disappear while hurt or native interaction poses invalidate sockets.

`prepare-equipment` converts the user's installed PTDE models and textures into
private `SCPROP1` packs. Canonical prop coordinates are metres, with +Z along the
sword blade or out of the shield front. The native grip remains the origin and
bone rest transforms are applied during preparation. Each pack contains the
8-byte `SCPROP1\0` magic, four little-endian uint32 fields (vertex count, triangle
count, texture width, texture height), six float32 bounds, 32-byte vertices
(position3, normal3, UV2), uint32 triangle indices and RGBA8 texture pixels.
The manifest maps native equipment rows to model IDs and records file hashes,
bounds, blade length and source executable identity. The installer validates
counts, lengths, indices, finite bounds and hashes before a rollback-backed copy.
No generated pack or native texture belongs in Git.

Native passive reactions (including a guard break with zero HP loss) take
temporary movement ownership before any outgoing contact is evaluated. The host
follows native displacement using `SC_POSE_STAGGER`, cancels equipment work and
waits for the verified native active-action flag to recover. Adapters fit that
pose to their own hurt animation; it is not a guest physics tick. No attack input
or residual guest velocity is submitted during the native reaction.


## Native parry and critical ownership (optional ABI4 extension)

`SC_CAP_NATIVE_COMBAT` (128, manifest `native_combat`) requires equipment support.
No struct layouts change. `SC_EquipmentPose.swing` additionally accepts
`SC_EQUIPMENT_BACKSTAB` (3), `SC_EQUIPMENT_RIPOSTE` (4), and
`SC_EQUIPMENT_PARRY` (5). `SC_POSE_NATIVE_COMBAT` preserves the most recent
equipment presentation, returns matching attachment matrices, follows the
supplied host root/yaw, and reports no attack/interaction/locomotion eligibility.
An adapter must not run its guest attack simulation during these poses.

The PTDE host sends a bounded native LT/parry or RB/light-attack pulse, observes
native animation layers, and locks guest locomotion until native recovery.
Nearby rear targets, observed parried targets, and guard+attack are input-routing
hints only: the game's native evaluator must approve the throw. An ineligible
critical attempt can become an ordinary native light swing. There is no extra
framework damage or stamina debit during this ownership interval. Death, actor
change, menu takeover and invalid state clear or transfer ownership; focus loss
cancels queued input, and return requires native idle plus focused quiet time.
Held attacks cannot spill into an extra guest swing afterward.

Mario supplies authored critical thrust/chop keys and an outward shield sweep.
Presentation reads elapsed native track time; its normalized visual schedule is
an approximation and does not alter native hit frames. It is not native skeleton
retargeting. Native contact volumes remain native during these sequences.
The first supported scope is one-handed fitted melee weapons and small/medium
parrying shields; unarmed Mario attacks retain the independent guest moveset.

## Native pyromancy (optional ABI4 extension, framework 0.8)

`SC_CAP_PYROMANCY` (256, manifest `pyromancy`) requires `native_combat` and
equipment support. It adds equipment kind `SC_EQUIPMENT_PYROMANCY` (8) in either
hand and actions `SC_EQUIPMENT_CAST_RIGHT` (6) / `SC_EQUIPMENT_CAST_LEFT` (7).
No struct layout changes. The adapter owns the caster prop and may change its
costume from the selected equipment. Native interaction/hurt/death posing may
hide the prop while retaining the current costume; a subsequent equipment
snapshot or instance destruction clears the selection. None means ordinary
appearance. Callbacks do not create a projectile, grant a spell, debit a charge,
or modify native memory.

PTDE identifies the native flame by its verified weapon category, animation
category and CanCastPyromancy flag, including upgraded variants. Right attack
casts from the right flame; the parry input casts from a left flame. The native
RB/LB pulse reads the current saved keyboard binding or routes to the selected
controller. Native spell selection, attunement, charges, flame scaling, cast
eligibility, projectile creation/collision and damage remain authoritative.
Ordinary jumps remain available; initiating casts requires grounded eligibility.
No extra guest contact attacks or stamina debit run during casting ownership.

The host presents the observed native windup/release track with a normalized
visual phase. This does not change native cast speed or interruption/recovery.
Fireball is the initial live-test target; other pyromancies require additional
presentation and gameplay coverage. Sorcery and miracle catalysts are not added.

## Optional camera profile (framework 0.8.1)

The separate `SoulsCharacterGetCameraProfile(version, size)` export returns a
validated `SC_CameraProfile` (version 1). This leaves ABI4's existing structures
unchanged. Missing export keeps the native camera. A profile may request a
distance multiplier in [1,2]; adapters never write camera memory. The manifest's
`camera.distance_scale` is emitted as `characterCameraProfile` for adapters that
export this optional getter. Mario requests 1.5; other characters remain unchanged.

The DS host applies this to the live LockCamParam camera distance, preserving
native camera collision, interpolation, FOV and lock-on targeting. It snapshots
each row, avoids cumulative multiplication, and restores only its own values
when the character releases control. Directory position/ID/offset identify rows
because the stock table contains duplicate ID 200. Replaced tables are not written
through stale references. External edits revoke ownership of the affected value.
`[Camera.<character-id>] DistanceScale` in mariosouls.ini overrides the default;
1 restores native distance. Invalid overrides select 1. No param file is edited.

## Optional projectile profile (framework 0.8.2)

`SoulsCharacterGetProjectileProfile(version, size)` returns a validated
`SC_ProjectileProfile`, version 1. The main ABI4 layout remains unchanged.
Omitting the export retains native spell behavior. Currently the DS1 PTDE host
supports `SC_PROJECTILE_FIREBALL`: the ordinary attuned Fireball cast through a
Pyromancy Flame in either hand. Other spells and other casters remain native.

The adapter declares cast-speed multiplier, horizontal projectile speed, gravity,
upward bounce speed, lifetime and initial height above feet. Distances use cm;
timing uses seconds. Adapters supply policy only and cannot access game memory.
Mario requests 3.5x casting, 1200 cm/s flight, 2400 cm/s² gravity, 600 cm/s bounce,
a three-second lifetime and 75 cm initial height. Native aim supplies horizontal
direction; the custom projectile does not home vertically onto locked targets.

The host identifies the caster through the native entity-handle table. Only a
new native Fireball belonging to the current character cast is enrolled. Native
pool slots are revalidated and cleared on reuse. The same native projectile
bounces using real map contacts; there is no replacement damage engine, duplicate
projectile spawn or spell-charge refund. Wall, ceiling and character impacts,
explosions, fire damage, flame upgrades, resistance and rewards remain native.
Adjacent floor-triangle reports are merged for 75 ms within a 12 cm height band;
vertical walls are never included. Flight ends through native expiry or impact.

Casting accelerates only the observed Fireball windup/release animations, with
a compare-and-restore lease on the current actor's animation rate. Completion,
interruption, death, interaction and mode handback release the lease. Source
pointers and actor identity are checked before restoration; external rate edits
revoke ownership. `[Projectiles] Enabled=0` in mariosouls.ini disables enrollment
and cast acceleration; already-launched shots finish their bounded flight.

## Optional native impact profile (version 1)

`SoulsCharacterGetImpactProfile(version, size)` returns `SC_ImpactProfile` or null
for incompatible requests. ABI4 is unchanged. `attackCount` must match the main
descriptor; `knockbackCm` indexes the same attacks. Zero leaves an attack on its
existing native path. Distances are bounded to 200 cm, cooldown to 1000..5000 ms,
and the resistant-poise threshold to 20..200. Invalid profiles opt out.

DS1 reads target current/maximum poise and its native collision capsule. At a
possible poise break, requested displacement scales by `1 - maxPoise/threshold`.
At/above the threshold, unknown shapes, radius above 80 cm or height above 280 cm,
the enhanced attack requests zero displacement. Native guard, immunity and reaction
remapping remain final authorities. The host writes only its own attack row and
pre-defense packet, never target position, velocity or poise. A target-specific
cooldown prevents repeated enhanced pushes; punches do not consume that cooldown.
The cooldown is conservatively spent on a push request, even if native defenses
later reject it. Target identity and actor reference are checked on reuse.

Mario opts in for combo kick, jump kick and slide kick. Other guest attacks,
equipped weapons, criticals and projectiles keep their existing impact paths.

## Optional world launcher (version 1)

`SoulsCharacterGetLauncherAPI(version, size)` returns the exact bounded
`SC_LauncherAPI` in `sdk/launcher_api.h`; incompatible requests return null.
ABI4 remains unchanged. Adapters without this export have no launcher objects.
Local positions and meshes use centimetres, Y up; yaw/pitch use radians.
`begin` requires an eligible consumed interaction close to the player. `step`
alone advances opening, aiming and flight; `query`, `texture` and `mesh` are
read-only observations. `cancel` is idempotent and restores guest visibility.
Mesh buffers and atlas are host-owned, with explicit capacities. No native
pointers, addresses, HP, quest, inventory or enemy writes cross this API.

The host owns placement, verified ground and line-of-sight, interaction priority,
input arbitration, native collision and scripted camera submission. Original
native prompts win over custom objects. Opening/aiming consume equipment input;
the entry press cannot fire. Damage, death, native takeover, focus loss, map or
character changes revoke the session. The camera request is actor/camera-bound,
expires after 150 ms without refresh, and replaces the gameplay camera's exported
matrix. The normal follow-camera transform is never overwritten. Both controller
sticks can aim; right stick takes priority, and native camera input is blocked
under the same expiring ownership. Guest-specific raw stick conventions remain
inside the adapter.
Flight still uses the shared motion bridge, native HP and fall-damage policy.

The host currently supports one launcher/operator pair at a Firelink placement
and an opt-in Asylum QA placement. Operator unlock state is stored per placement
in local mod configuration and shared between saves; it is not a native quest
flag. This is a bounded launcher capability, not a general imported NPC engine.

## Optional native starting class (version 1)

SoulsCharacterGetClassProfile(version, size) returns a static SC_ClassProfile
from sdk/class_api.h only for an exact version/size handshake. The profile
contains bounded UTF-8 display strings, a semantic native archetype, and preview
height in centimetres/yaw in radians. Invalid or absent profiles opt out; ABI4
is unchanged. The host owns class mapping, native initialization and inventory.
Adapters supply neither native class IDs nor memory addresses.

Creation uses a temporary pose-only guest instance isolated from world ownership.
The host destroys it before entering gameplay. The native saved class requests
the character once per loaded actor, preserving manual overrides during that
actor lifetime. No retroactive inventory is granted to existing saves.
See STARTING_CLASSES.md for PTDE mapping, activation and validation.


### Host render dimensions

Adapters submit world-space mesh data, not a resolution-specific framebuffer.
The DS1 PTDE host captures a current-frame scene depth and sizes its transparent
character target to that surface. It composites into the native UI target
before text/sprites; that intermediate target need not match the backbuffer.
Display-size changes recreate the layer; device reset releases default-pool
resources. There is no after-UI fallback that would cover prompts with characters.
The stock render-graph selector excludes square shadow/portrait passes, keeps
the largest rectangular matching color/depth pair, and ignores downsampled
passes. Multisampled and replacement render graphs require separate validation.


### Native lethal terrain and protected landings

PTDE collision caches now support MSC2: magic, little-endian uint32 triangle
count, then records of uint32 native hit-filter ID and nine float32 coordinates.
MSC1 still loads, but has no lethal-surface metadata and must be regenerated to
receive this fix. The exporter preserves hit filters from the owned MSB data.
Death-camera surfaces (13) do not enter guest geometry. Lethal-fall/kill surfaces
(14/15) remain queryable for guest engines requiring a floor underneath; the host
checks swept native feet against these surfaces before allowing another gameplay
step. Contact sets native HP to zero and begins the existing native-owned death
lifecycle, bypassing landing multipliers/caps. No adapter writes HP. Ordinary
landings retain the profile's reduced/nonlethal damage. A position below the
host's existing -1000m world bound now kills rather than merely disabling the mod.

Event-controlled collisions remain native-owned. When an event argument's target
cannot be resolved, all named collision candidates are conservatively excluded
from the static cache; their current state is not guessed. Hazard sweep history
resets at native-motion handbacks so teleports/ladders cannot create a false hit
along an old-to-new straight line. ABI and guest contracts are unchanged.
