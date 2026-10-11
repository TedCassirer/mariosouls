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

The current PTDE implementation supports right-hand straight swords with the
native straight-sword animation family and left-hand small/medium shields, in
one-handed stance. It supplies two alternating ground light swings and held
guard. It does not implement parry or two-handed moves; other weapon families
and temporary weapon-buff parity are outside the verified scope. Mario provides
the first fitted adapter. Other adapters must opt in and implement their own fit.

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

`SC_EquipmentPose` has right/left kinds (none, sword, shield), swing 0/1/2, normalized
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

Current authored sword timing is 24 ticks: windup 0..4, contact 5..12, recovery 13..23.
Late recovery accepts one new press; holding attack cannot repeat forever. Gear
change, damage, ownership loss or ineligible state cancels without refund. No
stamina means no swing. Guarding routes only the native guard button, slows guest
ground movement, and displays a raised shield only after native guard confirms.
Jump/crouch/attack/interaction release guard. Blocking never refunds HP or sets
NoDamage; native guard stamina, shield defenses and guard breaks remain native.

Blade contact sweeps between consecutive valid hand poses against native actor
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
