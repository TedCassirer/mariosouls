# SM64 cannon — framework 0.9.0

The optional SDK world-launcher capability places a cannon and pink Bob-omb Buddy
beside Firelink's central bonfire, toward the ruined arches. Available in Mario
mode; adapters without the capability do not show these objects.

## Controls

1. Stand beside the Buddy and press controller **A** or keyboard **Q** to unlock.
2. Approach the cannon and press **A / Q** to enter. Opening takes about five seconds.
3. Aim with **left stick / WASD**, fire with **A / Space**, leave with **Y / Q**.
   Release the entry button before firing; a held entry cannot launch.

Native interaction prompts take priority over custom objects. The launch uses
normal collision and native health, including Mario's existing fall-damage
reduction. No cannon invulnerability or second health pool is added.

## Fidelity and ownership

Original libsm64 act_in_cannon, act_shot_from_cannon, sounds, aim clamps, muzzle
displacement, velocity and gravity run at 30 Hz. Three missing opening actions
(4, 6, 5) are staged unchanged from pinned n64decomp/sm64 source at build time.
A persistent per-Mario object supplies usedObj; flight uses the existing motion
bridge rather than a teleport or a second character.

Base, barrel, lid, pink body, eyes, feet, winding key, textures and Buddy joint
animation channels come from the user's US ROM. The full SM64 object scheduler,
Buddy dialogue/AI and save system are not ported. Framework proximity interaction
unlocks the placement instead. Buddy faces the view and plays its ROM idle motion.

The host supplies an SM64-style circular sight and a verified native scene-camera
hook. Native terrain and guest meshes share the camera; the follow-camera matrix
is restored immediately after render submission. Requests expire after 150 ms
without renewal and are bound to the current camera and actor. Damage, death,
focus loss, native takeover, character or map changes cancel opening/aiming.

Native HP, stamina, inventory, quests and rewards remain authoritative. Guest
adapters receive no native memory pointers. See CONTRACT.md and ADDING_A_CHARACTER.md.

## Configuration and build

[WorldObjects.FirelinkLauncher] in mariosouls.ini accepts X, Y, Z, Yaw
(metres/radians) and Unlocked. Defaults are (-45, -60.5, 56), yaw -pi/2.
The Buddy is 2.5 metres east. Both snap to verified floor within 2 metres;
invalid ground suppresses the kit. Unlock state is shared between local saves,
not a native quest flag. [WorldObjects] QALauncher=1 enables a separate guarded
Asylum test placement; this is disabled for delivery.

Use python tools/framework.py build --with-runtime, then test --asset <ROM>
--runtime <sm64.dll> and doctor. Pinned libsm64 and n64decomp dependencies are
verified. prepare-collision --map m10_02_00_00 --output <fresh-directory> exports
Firelink collision with five event-controlled colliders left native-owned.
Install through the supported workflow with the game closed. ROMs, caches,
binaries, saves, generated dependency code and recordings stay out of Git.

## Validation — 2026-10-11

- Full offline suite passed: existing ownership/combat/equipment/death/controller
  regressions plus bounded launcher state and mesh requests.
- Actual guest opening with held A cannot fire; a separate press launches.
  Pitch clamps to 80 degrees and yaw to +/-90 degrees. Launch speed is the
  original 100 units/tick; cannon gravity subtracts 1 unit/tick. Duplicate entry,
  hurt cancellation and restored visibility passed.
- All 150 Buddy animation/blink frames have finite bounded meshes. Invalid
  requests and small output buffers fail. ROM mesh counts: base 38, barrel 62,
  Buddy 80 and lid 2 triangles.
- Mario QA only, Asylum: Buddy unlock, entry, matching scene camera, launch,
  upper-world contact, landing and ordinary controls. Native HP 594 -> 565 on
  landing; debug protection flags stayed zero. Re-entry, Q cancel and normal
  jumping were observed.
- Firelink: Buddy prompt/unlock, opening, aiming camera, launch and landing.
  Recorded Z 55.99 -> 78.91 m; peak Y -9.29 m from -59.45 m. Native HP 594 -> 565,
  with no-hit/no-damage/no-death flags all zero. Unlock survived save/reload.
- Buddy eyes, original joint motion and native/custom HUD ordering were checked
  visually on the installed final build.
- Opening the native inventory menu while aiming cancelled the sight, restored
  Mario and the follow camera, and closing it returned to normal movement.

Private evidence: work/cannon-qa. framework/qa/launcher_smoke.py records bounded
real keyboard navigation and actions. The observer explicitly allows Firelink
only when requested and retains the Mario QA name/stat guard. Combat tools still
default to Asylum. The optional Firelink fixture reset is limited to the bonfire
area and requires focused, living native control outside menus/interactions.

Controller routing passed offline; the controller was disconnected during live
cannon checks. Enemy hits and lethal damage while aiming, and every interruption
path, have not all been verified live. Adapter hurt cancellation is tested.

Dark Souls was not designed for SM64 launch reach. Some trajectories reach roofs,
awkward ledges or out-of-bounds areas. The recorded high shot landed beside upper
ruin geometry and required resetting QA for the next check. This is a functioning
launcher integration, not a claim that every direction is a safe shortcut.
