# Playing MarioSouls

## Select Mario

Mario replaces the Pyromancer class. Select **Mario** during character creation; the model also appears in the appearance panels. Native Pyromancer stats and tutorial progression remain. Equipment is acquired through the tutorial, including the Flame; the mod does not grant a Flame immediately or migrate existing inventories.

Existing Pyromancer saves are recognized as the same class. Class intent restores Mario after loading, respawn and area changes. F6, or holding both stick buttons, switches to the native character and back. A manual off choice applies to the current life/area before saved-class defaults are reconsidered.

## Controls

| Action | Controller | Keyboard / mouse |
| --- | --- | --- |
| Move | Left stick | WASD |
| Camera | Right stick | Mouse |
| Jump / interact at an eligible prompt | A | Space to jump; Q to interact |
| Attack / right-hand cast | B or RB | Left mouse |
| Left-hand cast / shield guard | LB (L1) | Right mouse |
| Shield parry | RT (R2) | Tab |
| Crouch / airborne ground-pound input | LT (L2) | Left Ctrl |
| Use selected item | X | E |
| Mario / native toggle | Hold both stick buttons | F6 |

A handles world prompts; Y does not talk, open doors or activate bonfires. Y still cancels an active cannon sequence. Native menus and native-character mode retain their normal controls. An interaction consumes its A press until release, so closing a menu does not immediately trigger a jump. Existing bindings/game options can affect native input.

## Gloves and equipment

Equip **Mario's Gloves in the active right hand** for damaging Mario punches, kicks, dives, stomps and ground pounds. Empty hands do not enable their damage. Navigation remains available without combat stamina costs; damaging attacks require native stamina.

The gloves are the native Caestus with Mario-specific text, following normal +0..15 reinforcement with native titanite, souls and ember requirements. The Mario class's tutorial axe pickup maps to the gloves. Existing characters receive no automatic items; a normal Caestus bought later can fill the same role. Infused or left-hand gloves do not enable the Mario moveset. HUD ATK still displays native fist stats rather than Mario move power.

Supported one-handed daggers, straight/curved swords, greatswords, axes and hammers/maces use native weapon damage and fitted Mario swings. Shields use native guard/parry. Backstab/riposte attempts require a suitable weapon and a valid native opportunity; guard+attack also requests a critical attempt. Two-handing and every weapon family are not covered.

Selecting an equipped Pyromancy Flame gives Fire Mario colors and a held Fire Flower. Attune/select Fireball normally, then cast with LB/L1 (right mouse) for the left hand or B/RB (left mouse) for the right hand. LT/L2 remains crouch/ground pound. Casts use native charges and flame upgrades. The mod adds a faster, bouncing Fireball profile. Other pyromancies are not individually verified. More detail: [equipment](../framework/characters/mario64/EQUIPMENT.md).

## Interactions and hazards

Talk, doors, bonfires, ladders and Estus use native actions while Mario remains the intended presentation. Menus suspend guest movement. Some scripted/cinematic animation fitting is approximate, and cutscene coverage is incomplete.

Native HP, armor, status buildup, healing, death, rewards and quest state remain authoritative. Ordinary landing damage is heavily reduced; pits/kill planes still kill. Ordinary jumping does not add invulnerability. Equipment load does not add a custom Mario speed/animation penalty.

## Diagnostics

F8 toggles an initially hidden collision overlay, F9 cycles views and F10 toggles through-walls drawing. F6 is independent of the overlay. The overlay shows approximate body/contact and attack information, not exact native per-bone hurtboxes or all enemy weapon shapes. See [overlay details](../framework/docs/DEBUG_VIEW.md).

For problems, use [troubleshooting](TROUBLESHOOTING.md) and [bug reports](BUG_REPORTING.md). The complete behavior/tuning inventory is [Features and tweaks](FEATURES_AND_TWEAKS.md); it also records unverified areas. Use separate disposable saves for testing.
