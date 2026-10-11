# Mario equipment presentation

The Mario adapter layers authored arm rotations over the ordinary guest animation
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
same movement/action trace. Axes and hammers use a lifted diagonal chop that
returns along the same side; cutting families use alternating sweeps. The shared
host timing stretches windup/recovery for heavier weapons and shortens dagger
swings. Native stamina and weapon damage remain host-owned. These authored
upper-body overlays do not reproduce the complete Dark Souls moveset.
