# Mario equipment and animation fitting

The host selects native items/actions and owns stamina, damage and guard/critical success. Mario's adapter supplies neutral equipment presentation and same-frame hand sockets. It does not import a Dark Souls rig or moveset wholesale.

## Gloves and melee families

Mario's normal punches/kicks require the semantic unarmed-focus profile, mapped by PTDE to normal Caestus +0..15. Gloves use native inventory/reinforcement and Mario-specific text; the native fist icon/ATK display remains. The [feature inventory](../../../docs/FEATURES_AND_TWEAKS.md) records attack power/cost/defaults.

Supported one-handed kinds are straight sword, dagger, curved sword, greatsword, axe and hammer/mace. Each has two authored upper-body clips with anticipation, strike and recovery: compact dagger cuts, sword sweeps, larger greatsword chops and raised axe/hammer strikes. Torso/head fitting follows the arms while guest feet, root motion and action clocks remain intact.

Equipment is a pose layer over the current guest animation. It is suppressed by incompatible scripted/damage actions. A held weapon/shield keeps the ordinary idle head-turn cycle but prevents sleeping; equipping during sleep uses the guest wake action before weapon actions become eligible. Unequipping restores ordinary idle behavior.

## Hand sockets and contact

Sockets come from the graph traversal that produced the returned mesh and follow native position corrections. They are not sampled from another frame. The owned guest geometry uses anatomical labels opposite to the host view convention, so fitting is mirrored through actual shoulder/hand frames.

The host uses authoritative sockets/contact timing, while rendered interpolation is visual only. Native props are prepared from owned PTDE assets and drawn at those sockets. A visually plausible pose does not establish native damage, blocking or interruption correctness.

## Shield, parry and critical presentation

LB/right mouse guards; RT/Tab requests the equipped shield's native parry. LT remains crouch/ground pound. With a supported weapon, B/RB/left mouse can request a backstab behind an enemy or riposte against a parried enemy; guard+attack also explicitly requests a critical attempt.

Native rules decide success and paired timing. Mario uses authored shield/thrust/chop poses with guest attack contacts suspended. Unsupported families, two-handing and unarmed criticals are not implemented by this extension. Temporary weapon-buff parity is not complete.

## Fire Flower

An active Pyromancy Flame produces a white hat/shirt, red overalls and an authored held Fire Flower. Switching away from both active flames restores normal Mario. Interactions can hide hand props while keeping the outfit. Inventory/HUD still use the native Flame name/icon.

Attune/select Fireball through native menus. Right hand casts with B/RB/left mouse; left hand with LB/L1/right mouse, the same primary input that guards with a shield. RT/Tab only requests shield parry; LT/L2 remains crouch/ground pound. Each cast needs a fresh press: holding a button or changing equipment does not create repeated cast requests. Native spell uses, attunement, reinforcement, collision, damage and rewards remain authoritative. The optional projectile profile adds faster casting and bouncing Fireball behavior; other pyromancies are not individually verified.

The flower mesh is authored in `fire_flower.h`; `author_flower.py` fits original arm keys against the owned skeleton. No downloaded Nintendo flower model is bundled.

## Authoring and private previews

`author_equipment.py` fits authored hand/weapon goals with an elbow hinge and reachable wrist targets; `equipment_keys.h` stores the resulting original keys. The runtime builder copies keys into its generated staging tree without changing the pinned checkout. Windup/strike/recovery use continuous fitted transitions rather than stopping the arc at an impact key.

The manual review command is:

```powershell
python tools/framework.py preview-equipment --character mario64 --asset <owned-rom> --runtime <built-sm64.dll> --equipment-directory <private-props> --output <fresh-dir>
```

Add `--actions casting` for cast poses. Preview tools export meshes/glove transforms and render front/rear contact sheets using the host pose timing. These outputs contain owned-ROM/native geometry and stay outside Git. They establish pose appearance, not gameplay compatibility.

Update this guide, [Features and tweaks](../../../docs/FEATURES_AND_TWEAKS.md) and the changelog when presentation, supported items, action semantics or timing change. See the [SDK contract](../../docs/CONTRACT.md) and [manual checklist](../../../docs/TESTING.md).
