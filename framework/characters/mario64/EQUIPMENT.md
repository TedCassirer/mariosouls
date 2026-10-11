# Mario equipment presentation

The Mario adapter layers authored upper-body rotations over the ordinary guest animation
without changing movement, attack actions or animation timing. The host owns
equipment selection, attack timing, stamina, native damage and guard confirmation.
Sword and shield sockets come from the graph traversal that produced the current
mesh and follow host position corrections. Scripted interactions and damage
reactions suppress the equipment layer.

The guest geometry's anatomical labels use the opposite handedness from the
host's view convention. Authored poses are mirrored through the actual shoulder
frames and bound to the rendered native-right and native-left gloves. Socket
translations are still captured from those exact bones; meshes, visible hands
and weapon collision therefore agree. No host character-specific switch is used.

One deliberate idle variation applies while a sword or shield is carried: Mario
keeps his normal idle head-turn cycle but does not fall asleep. Equipping an item
while he is asleep runs the guest's normal wake animation before enabling weapon
actions. Unequipping restores ordinary sleep behavior. This does not change running
speeds, jumping, landing, or unarmed attacks.

`equipment_test.cpp` compares equipped and unequipped movement/action traces,
checks attachment transforms and correction/lifecycle rules, and tests long idle
and waking. Its optional preview output contains geometry extracted from the
user's own ROM and must remain private and outside Git.

## Melee-family extension

Mario opts into `SC_CAP_MELEE_FAMILIES`. All six supported kinds preserve the
same movement/action trace. Each family has two fitted upper-body clips, with
independent anticipation, strike and recovery poses. Daggers use compact cuts,
straight and curved swords alternating sweeps, greatswords a broad chop and
return cut, and axes/maces raised diagonal/vertical chops. Torso rotation and
lean follow the arm; the head counters the chest and looks into the strike.
Feet, root motion and guest animation clocks remain unchanged.

The contact phase is eased as one continuous arc through its middle. It no
longer decelerates to a stop at the impact key. Windup and recovery have their own
smooth transitions. Every clip starts/ends at exactly the same ready-arm key.
Native damage, stamina and shared host contact-window timing are unchanged.

`equipment_keys.h` contains original authored keys. `author_equipment.py` fits
authored glove/weapon goals using the owned-ROM skeleton, an elbow hinge and
reachable wrist targets. The builder copies those keys into its generated runtime;
it never changes the pinned libsm64 checkout. This is an authored animation pass,
not an imported Dark Souls moveset.

## Repeatable visual review

`python tools/framework.py preview-equipment --character mario64 --asset <owned-rom>
--runtime <built-sm64.dll> --equipment-directory <private-props> --output <fresh-dir>`

The SDK fixture exports both swings for all six families at the host's actual
30 Hz timing. The shared QA renderer produces front and back contact sheets from
the returned mesh and glove matrices, including weapon props at the same model
scale. Dumps contain owned-ROM geometry and stay private, outside Git. These
previews establish visual behavior, not native damage or live-game performance.


## Parries and criticals

Mario opts into the shared `native_combat` capability. RT on a controller or Tab
on a keyboard requests the equipped shield's native parry. LT remains Mario's
crouch/ground-pound input. With a supported one-handed weapon, B/RB/left-click
requests a native backstab behind a nearby enemy, or a riposte in front of an
enemy in its native parried reaction. Step closer after parrying if the enemy
recoils out of reach. Guard+attack also explicitly requests a native critical
attempt. Native rules decide whether it succeeds; an invalid opportunity may
produce an ordinary native weapon swing.

The model stays Mario. Native combat poses keep the equipment sockets and render
original authored critical thrusts/chops and a shield sweep. Damage, stamina,
protection and enemy animation stay native; guest attack contacts are suspended.
Visual timing follows the native track clock with an approximate normalized
schedule. Unsupported shield/weapon families, two-handing and unarmed criticals
are not implemented by this extension.
