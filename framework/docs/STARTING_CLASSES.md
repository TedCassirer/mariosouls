# Mario class and persistent selection

The optional version-1 class profile in [class_api.h](../sdk/class_api.h) provides a name, description, semantic archetype and preview framing. PTDE maps the firecaster archetype to Pyromancer. The adapter never receives a native class address or edits inventory.

## Player behavior

Pyromancer is displayed as **Mario**, with an animated guest preview. Existing saved Pyromancer class identity also requests Mario. Stats, armor and tutorial progression remain native. The initial record does not grant a Flame; it is acquired during the tutorial.

Mario's glove integration changes the class preview weapon and class-specific tutorial axe pickup to normal Caestus/Mario's Gloves. Existing inventories are not migrated or auto-equipped. Physique/face controls still describe the fallback human, but the selected class's guest preview remains visible.

F6/manual off is an override for the current life/area. Loading/respawn, a map boundary or actor reload reconsiders the saved-class default. An unknown class read during streaming can retain a confirmed request; unrelated classes must not inherit the previous save's intent.

`[Framework] StartingClass=0` disables class replacement on the next launch while preserving manual character mode.

## PTDE integration

- Saved Pyromancer identity is native class byte 7. Selection requests the corresponding native preview initializer.
- Actual `CharaInitParam` 2007 remains the native start. Glove mapping applies to preview record 3007 and tutorial item lot 1810260; do not describe that as replacing the actual initial inventory record.
- Class text is scoped to the appropriate native menu FMGs. The profile name is cosmetic; saved class identity remains the persistent source of intent even if text replacement is unavailable.
- Preview detection covers class/trait panels and the face editor. The host recognizes native portrait render targets/UI quads, follows the actual pane bounds and restores graphics state. It does not replace an arbitrary UI texture.
- Preview instances/buffers are isolated from world ownership, use pose-only generation and are released before another guest lifetime takes over.
- Movement waits for a settled living/safe native identity. Pose-only appearance can precede movement ownership without input filtering, body mutation or outgoing contacts. Missing/invalid replacement data retains native visibility.

Native addresses, menu fields and hook evidence are documented in the [PTDE inventory](../hosts/ds1ptde/PORT_CHECKLIST.md) and implemented in `starting_class.h`, `class_preview.h` and `unarmed_equipment.h` there. Another host must rediscover its class/preview mapping rather than reuse PTDE IDs.

## Verification and maintenance

Manually check class selection, trait submenus, creation, tutorial pickups, load/reload, death/respawn, transitions, F6 and an unrelated class. No automated character-creation/save-editing infrastructure is maintained. Record the exact build and update [Features and tweaks](../../docs/FEATURES_AND_TWEAKS.md) and [CHANGELOG.md](../../CHANGELOG.md) for behavior changes.
