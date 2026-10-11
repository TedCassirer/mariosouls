# Add a character

Wind Waker Link now demonstrates the asset-driven route: an original model and
sampled animations with an authored controller. Mario still demonstrates an
embedded guest engine. These routes share host collision, menus, combat and input.
Neither automatically imports another game's mechanics.

Adapters may provide `prepare_assets.py --input <owned-disc> --output <private-pack>`.
Invoke it through `python tools/framework.py prepare-assets --character <id>` with
those arguments. The optional SCMESH01 loader and kinematic queries live in core;
game formats, clip names and equipment attachment rules remain character-specific.
Declare `assets.format: SCMESH01` for exhaustive clip and malformed-pack tests.
A character-local `test.cpp` is automatically run by the supported test workflow.
Use `texture_modulate` and `alpha_test` manifest capabilities for ordinary textured
models with cutout materials; do not add character-name branches to the renderer.

Run from the repository root:

```powershell
python tools/framework.py new-character my_character --name "My character"
python tools/framework.py build --character my_character --adapter-only
```

This creates `framework/characters/my_character/character.json`, `adapter.cpp`
and a README. The generated adapter is a procedural triangle that demonstrates
the ABI. It is deliberately marked `scaffold`, and installation rejects scaffolds.
It is not a Link or DK implementation. Never change the status just to bypass
that check.

1. Choose and implement a usable guest movement/animation engine and asset loader.
   A model alone cannot reproduce another game's character mechanics. Read assets
   from the owner's local files; never embed a ROM, copyrighted meshes or source
   tree in this repository. For a separately built runtime, provide a local
   `build_runtime.py --output <directory>` next to the adapter.
2. Translate input, coordinates, ticks, collision and mesh output to the SDK.
   Implement bounded buffers, error handling, destroy/recreate and `resolveMotion`.
   Corrections must translate the existing mesh without advancing the animation,
   clip only velocity into supplied collision normals, and preserve attack events.
3. Map the character's attack phases and contact probes to semantic attack IDs.
   Define power/stamina/poise/range in the manifest. Use the shared damage and
   progression code; do not read or write Dark Souls memory from the adapter.
4. Implement host-owned interaction poses and clean resume on the same instance.
   Test interrupted actions and rejected prompts as well as successful actions.
   Report `SC_CAN_INTERACT` and `SC_GROUNDED` accurately. The shared controller-A
   policy chooses native interaction at an eligible prompt or guest jump elsewhere;
   adapters must not independently turn that interaction press into a jump.
   Opt into `death_pose` only when `SC_POSE_DEATH` actually plays and holds a
   character-specific death animation. Keep guest death/respawn logic disabled;
   the host still owns the dead actor. Test terminal hold and new-life reset.
5. Run a full `build --character <id>`, then `test --character <id>` with required
   `--asset` and `--runtime` paths. The common suite checks lifecycle, bounds,
   floor, movement/jump, all poses, resume and damage/audio callbacks. Add engine-
   specific test fixtures where the common locomotion expectations do not fit.
6. Mark implemented only when the adapter is real, document its limitations,
   install through the checked installer, and validate with the dedicated test
   save. Check native damage, stamina, movement, UI order, world collision and
   actions in the game. Offline tests are necessary but cannot prove those hooks.

Keep general fixes in `core/` or `hosts/ds1ptde/`. Add SDK capabilities when a new
character needs an ability the interface cannot express. Do not branch on a
character ID in the host. A new character normally adds its folder and tests;
it should not duplicate the renderer, door logic, save guard or combat routine.

Link/OoT would need an OoT-compatible runtime/asset and animation implementation;
DK/DK64 would need its own. Neither is present. Research their runtime availability
when implementing those ports; Mario's libsm64 cannot run them. Extra buttons,
new weapon families, aiming, projectiles, transformations or unusually large
collision shapes require explicit framework extensions. Current v4 supports one
active character, a configurable native body, movement with jump/attack/crouch/guard
inputs, native interaction poses and optional sword/shield fitting. It does not
promise every game mechanic out of the box.


Optional `body` manifest values: `model_scale`, `radius_cm`, `height_cm`,
`hurt_radius_cm`, `hurt_height_cm`, `fall_damage_scale`,
`fall_damage_max_hp_fraction`, `nonlethal_falls`. Dimensions default to zero
(native unchanged); scale/multiplier/cap default to1. Keep private engine wall
and ceiling tests consistent with the host body; never scale velocities to obtain
a smaller mesh. `build --with-runtime` builds the adapter's guest extension too.
Rebuild both host and adapters for ABI4; earlier DLLs are rejected.

`probeContact` now receives a native body contact candidate, not necessarily the
enemy foot origin. Use that point for facing/action checks; do not infer target
size or multiply attack reach by target size in an adapter. Shared host geometry
handles large bodies, upper-surface automatic attacks, occlusion and native damage.
Prepared collision caches leave event-controlled barriers to native Havok; an
adapter must honor their live contact/resolution callbacks.

F8 enables the shared debug overlay; F9 cycles views and F10 controls occlusion.
It reads the adapter's existing descriptor and samples `probeContact` without
stepping or committing contacts. Keep that predicate free of side effects.
Use the capsule, contact, range and terrain views during integration; they do not
claim exact native per-bone hurtboxes. See DEBUG_VIEW.md.

ABI4 adds optional `equipment` capability. Implement presentation-only
`equipment` and same-frame `attachments` callbacks, and advertise
`SC_CAN_EQUIP_ACTION` only in safe grounded locomotion states. Keep the original
engine's movement tick; layer arm poses after sampling its normal animation.
Capture normalized hand sockets from the same graph traversal, then shift them
with motion corrections and rebasing. See CONTRACT.md for axes/lifecycle.

Add `equipment` to the manifest capabilities only after implementing both
callbacks. Without that capability, leave them null. Fit both authored swing
phases and the native-confirmed guard amount; do not infer protection from a
raised-arm pose. Clear sockets during unsupported hurt/interaction states and
on lifecycle changes. Keep item selection, native weapon power, reinforcement,
stamina and guard decisions in the host. An empty right hand must return to the
adapter's existing independent attacks. Mario currently supplies this fit;
Wind Waker Link does not gain equipment support merely by rebuilding for ABI4.

Provide `<character>/equipment_test.cpp` to verify paired locomotion with/without
equipment, actual pose differences, socket normalization, motion reconciliation
and lifecycle invalidation. The supported test workflow passes character.dll,
owned asset path and guest runtime path. Do not claim equipment support simply
because the host can read inventory; a fitted, tested adapter is required.

Private PTDE equipment models are prepared using `python tools/framework.py
prepare-equipment --output <private-cache>`, then installed using
`install --equipment-directory <private-cache>` alongside the character's usual
asset/runtime arguments. The installer verifies executable identity, manifest,
geometry bounds and hashes, and participates in the ordinary rollback transaction.
The initial host scope is one-handed straight swords, small/medium shields,
ground light swings and blocking. Preparing all supported assets is not proof
of in-game behavior for every item. Validate real hits, misses, stamina exhaustion,
front/rear shield impacts, guard breaks, equipment changes and native interactions
on the dedicated test save, and document unverified cases explicitly.
