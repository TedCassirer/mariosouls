# Wind Waker Link — first playable slice

Original GZLE01 model, textures, equipment and 25 skeletal animation clips are
converted privately from the owner's disc. The locomotion/combat controller is
authored for this framework. This is not an embedded Wind Waker engine and does
not reproduce its entire moveset or exact physics.

## Prepare and build

The local asset recipe uses DolphinTool 2609a, SuperBMD 1.3.11 and gclib
`4e06db0004df411f1c806ad39b0e9a18ac4a1de5`, plus numpy, Pillow and gclib's
imagequant dependency. See `asset_dependencies.json` for source and pin details.
Tools live in the ignored `work/tools/`; Python extras are in `work/python-deps/`.

```powershell
python tools/framework.py prepare-assets --character link_ww --input "C:\path\Wind Waker.rvz" --output work/link-ww/link.scmesh
python tools/framework.py doctor --character link_ww
python tools/framework.py build --character link_ww
python tools/framework.py test --character link_ww --asset work/link-ww/link.scmesh
python tools/framework.py install --character link_ww --asset work/link-ww/link.scmesh
```

Installation requires the game closed and backs up the previous active adapter.
The recipe accepts only USA GZLE01. It does not emulate or launch the disc. Its
extracted source, atlas, pack and provenance receipt are private assets, never Git
content or release files. The framework references the pack at its local path.

## Controls and mechanics

- Existing toggle: F6 / hold both stick buttons. Camera/input routes are shared.
- Move: WASD / left stick. Jump: Space / A. Navigation jumps are manual.
- Sword: left mouse / B. Tap again during a swing to queue the next combo hit.
- Roll: move + Ctrl / LT. Spin: Ctrl + attack / LT + B from a grounded idle/run.
- Interact: Q / Y. Use item: E / X. Native menus retain
  their normal controls. Link's extracted clips present native interactions.

Walking/running reaches 330 cm/s, jump speed 390 cm/s, gravity 900 cm/s², roll
470 cm/s initially then 180 cm/s recovery. These are provisional authored values,
independent of stats. Rolls and jumps grant no extra invulnerability. Shield
geometry is carried but does not implement blocking. Automatic ledge jumps,
target lock, parries, bow, boomerang, sailing, guest voices/SFX and animated facial
expressions are not implemented. Original eye materials are flattened to a neutral
expression; lighting approximates the original through the shared renderer.

Combat uses native HP/stamina/defenses/rewards and the shared independent-power
formula. The three slashes use separate hit windows; spin has recovery through
tick 35. Exhausted swings can animate but deal no damage. Costs and multipliers
are initial tuning in the manifest, not claims of final balance. Ground speed,
jump height and body dimensions do not scale with Strength/Dexterity.

Shared `core/mesh_pack.h` loads bounded skeletal packs; `core/kinematic.h` supplies
triangle floor/ceiling/wall queries. Host Havok reconciliation handles native
contacts. Character-specific archive parsing, clip IDs, props, inputs and moves
stay here. `test.cpp` covers actual combos, roll interruption and body clearance.
Live verification coverage is recorded separately; offline tests alone cannot
establish native gameplay correctness.
