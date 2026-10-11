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
weapons, aiming, projectiles, transformations or unusually large collision shapes
will require explicit framework extensions. Current v3 supports one active
character, a configurable native body, three movement/combat inputs and generic native
interaction poses; it does not promise every game mechanic out of the box.


Optional `body` manifest values: `model_scale`, `radius_cm`, `height_cm`,
`hurt_radius_cm`, `hurt_height_cm`, `fall_damage_scale`,
`fall_damage_max_hp_fraction`, `nonlethal_falls`. Dimensions default to zero
(native unchanged); scale/multiplier/cap default to1. Keep private engine wall
and ceiling tests consistent with the host body; never scale velocities to obtain
a smaller mesh. `build --with-runtime` builds the adapter's guest extension too.
Rebuild both host and adapters for ABI3; earlier DLLs are rejected.

`probeContact` now receives a native body contact candidate, not necessarily the
enemy foot origin. Use that point for facing/action checks; do not infer target
size or multiply attack reach by target size in an adapter. Shared host geometry
handles large bodies, upper-surface automatic attacks, occlusion and native damage.
Prepared collision caches leave event-controlled barriers to native Havok; an
adapter must honor their live contact/resolution callbacks.
