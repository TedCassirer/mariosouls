# Character ABI version 2

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

ABI 1 adapters are rejected; rebuild both host and adapter. `resolveMotion` is a
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

Mesh arrays are unindexed triangles. Position/normal/color each have
`capacityTriangles * 9` floats; UV has `capacityTriangles * 6`. Do not write beyond
capacity. Set `numTrianglesUsed` on every successful step/pose. The descriptor
allows at most 16,384 triangles and a 4096×4096 atlas. Frame validation rejects
non-finite output, invalid attack IDs and excess mesh counts and restores native
control. Rendering uses the native camera/depth and composites before Dark Souls UI.
The current renderer uses one atlas and vertex colors, without a general material
or skeleton system. Large/multi-material characters require a shared SDK extension.

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

`SC_CAP_ITEM_PROP` tells the host the adapter draws its own held item during use;
it suppresses the native human-hand prop/effect. Add character-specific fittings
in the adapter, never in host code.

## Attack contract

`character.json` generates the compiled descriptor; editing installed JSON alone
does not change behavior. Attack index/name pairs identify moves. Report -1 when
no attack is underway. Distinct combo punches and finishing kicks need separate
IDs. Repeated instances of the same move must include an intervening idle sample;
v2 does not have an explicit attack-instance token. If a future engine can chain
identical attacks without that boundary, extend the SDK and shared window logic.

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

Power is independent of equipped weapons. Shared scaling currently uses base +
upgradeStep × level + strengthBonus + dexterityWeight × strengthBonus(dex).
The bonus awards 3/stat over 10 to 20, 2/stat over 20 to 40, then 0.5/stat to 99.
The native host inserts attack power before native defenses. Range, vertical bounds,
target count, multiplier, stamina cost and poise are manifest-driven; shared stagger
limits and equipment-weight costs apply downstream. The current host supplies
physical melee/area contact attacks. Projectiles, magic, shields and target-lock
abilities need new shared capabilities, not hidden character-specific hooks.

`rawAction`, `rawFlags` and related fields are opaque diagnostics only. No host
gameplay branch may inspect engine IDs. The architecture check catches direct
runtime imports and common forms of this dependency, but code review is still needed.

## Audio

With `SC_CAP_AUDIO`, `audio` writes interleaved signed 16-bit stereo at audioRate.
Lengths are sample **frames**, two int16 values per frame. Never exceed
capacityFrames, even if desiredFrames is larger. Return zero for insufficient
capacity or absent audio. Without the capability, provide a no-op callback.
