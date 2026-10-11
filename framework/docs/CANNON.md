# SM64 cannon

The optional SDK world-launcher capability places a cannon and pink Bob-omb Buddy
beside Firelink's central bonfire, toward the ruined arches. They are available
in Mario mode; adapters without the capability do not show them.

## Controls

1. Stand beside the Buddy and press controller **A** or keyboard **Q** to unlock.
2. Approach the cannon and press **A / Q** to enter. Opening takes about five seconds.
3. Aim with **either stick / WASD**, fire with **A / Space**, leave with **Y / Q**.

Release the entry button before firing; holding it cannot launch. Right stick
takes priority if both sticks move. Ordinary follow-camera input, lock-on and
mouse rotation are suppressed during aiming. Native interaction prompts take
priority over custom objects.

## Guest and host responsibilities

The guest runs libsm64's cannon entry/flight actions, sounds, aim clamps, muzzle
displacement, velocity and gravity at 30 Hz. Three missing opening actions are
staged from pinned SM64 decompilation source at build time. A persistent guest
object supplies cannon state; flight uses the framework motion bridge.

Cannon and Buddy meshes, textures and joint-animation channels are decoded from
the user's US ROM. The full SM64 object scheduler, Buddy dialogue/AI and save
system are not ported. Framework proximity interaction unlocks the placement;
Buddy faces the view and plays its ROM idle animation.

The host supplies a circular sight and camera export override. Aim rotates about
a fixed cannon eye using the same yaw/pitch as launch. The override leaves the
native simulation camera untouched, expires after 150 ms without renewal and
binds to the current camera and actor. Damage, death, focus loss, native takeover,
character or map changes cancel opening/aiming.

Launches use normal collision and native health, including Mario's fall-damage
reduction. No cannon invulnerability or second health pool is added. See the
[SDK contract](CONTRACT.md) and [adapter guide](ADDING_A_CHARACTER.md).

## Configuration and build

`[WorldObjects.FirelinkLauncher]` in `mariosouls.ini` accepts `X`, `Y`, `Z`, `Yaw`
(metres/radians) and `Unlocked`. Defaults are `(-45, -60.5, 56)`, yaw `-pi/2`.
Buddy is 2.5 metres east. Both snap to verified floor within 2 metres; invalid
ground suppresses the kit. Unlock state is shared between local saves rather
than stored as a native quest flag.

`[WorldObjects] QALauncher=1` enables a separate guarded Asylum test placement.
It is disabled by default and must remain disabled in delivered configuration.

Use the supported [build and test workflow](../../docs/DEVELOPMENT.md), including
`build --with-runtime`. Firelink collision preparation omits five event-controlled
colliders from the guest cache so they remain native-owned. ROMs, generated
caches, dependency code, binaries, saves and recordings stay out of Git.

## Verification and limits

Offline tests exercise actual guest opening and flight, fresh-button launch,
80-degree pitch and +/-90-degree yaw clamps, original launch speed/gravity,
duplicate entry, cancellation, visibility and bounded ROM meshes. Camera wrapper
tests cover native register outputs, a fixed eye with changing direction,
source preservation, unrelated cameras, expiry and actor replacement. Adapter
checks verify different headings, acknowledgements, muzzle offsets and velocity.

Historical live checks on Mario QA observed Buddy unlock, opening, launch,
landing, restored controls and menu cancellation in Asylum and Firelink. The
subsequent camera-pivot/input correction has offline coverage but still requires
live visual confirmation. Controller use, enemy hits/lethal damage while aiming
and every interruption path have not all been verified live.

See the [archived observations and camera investigation](https://github.com/TedCassirer/mariosouls/blob/53a8e61/framework/docs/CANNON.md),
[current coverage](VALIDATION.md) and guarded [launcher QA scenario](../qa/README.md).
Some launch trajectories reach roofs, awkward ledges or out-of-bounds areas;
this integration does not make every direction a safe shortcut.
