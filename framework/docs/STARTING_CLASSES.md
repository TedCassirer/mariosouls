# Native starting classes

Optional `SoulsCharacterGetClassProfile` (version 1, `sdk/class_api.h`) lets an
adapter supply a class name, description and preview framing. The host maps the
generic firecaster archetype onto its native class. Native initialization and tutorial progression are untouched.
Adapters continue to have no memory or inventory access.

## PTDE implementation

- Pyromancer is native class byte 7 at PlayerGameData + C6. Native selection at
  FA4E50 writes that byte and requests preview initializer 3000 + class.
- CharaInitParam 2007 is the actual start; 3007 is the menu preview. The host
  does not hook initialization or modify either record. The original 2007 starts
  with a broken sword and attuned Fireball, but no Flame. Flame, shield and axe
  remain native tutorial pickups. All original stats, armor and items remain.
- D175E0 reads a single FMG entry (EDX id, stack FMG). The hook replaces only
  132027/132327 in the menu-other base/patch FMGs, identified through MsgRepository
  +138/+1F0. This covers both the direct menu-list lookup and the common wrapper.
- Creation is isolated from world ownership: MenuMan +94 values 2/3/5/7 or the active face editor (+B0, +B4), absent
  native player, selected class 7, supported adapter, installed hooks. A temporary
  guest instance generates an idle mesh without gameplay ticks or controls. It
  is destroyed before world activation or when another class is selected.
- Both clothed and appearance panels use SHORT2 UI quads and 512-square ARGB
  render targets. Their observed bounds differ: (6600,1216)-(9568,4984) versus
  (6720,1720)-(9576,5056), in eighth-pixels. Match a large portrait quad within
  the right pane and require a render-target texture, excluding UI atlases/icons.
  Projection aspect follows the actual quad; rotation follows the active viewer.
  This includes physique, face, hair/color lists and advanced face customization.
  Native UI owns placement, clipping and composition. GPU resources restore
  device state and release on reset. Appearance controls still describe the
  fallback human; they do not reshape or recolor the guest mesh.
- Saved class is persistent identity, not a one-shot activation event. Class
  detection remains active even if the cosmetic name hook is unavailable.
  This also applies to existing Pyromancer saves; no items or stats are granted.
  Other classes retain native defaults and do not inherit the previous save's
  class request. Unknown class reads during streaming retain a confirmed class.
- Manual toggle-off remains available for the current life/area. Death/respawn,
  a map change or actor reload restores the saved-class default, including when
  the engine reuses actor pointers. Recovery/failure paths preserve pending intent.
- Movement acquisition settles identity for 500 ms and still waits for a living,
  safe native frame. Separate pose-only presentation supplies the guest model
  immediately when actor/camera/pose data are valid, even while spawn/menu/death
  or recovery keeps physics native. No movement, input filtering or combat runs
  in this presentation path. Host visibility suppression starts only after a
  validated replacement mesh exists, and restores on explicit off/teardown.
- Cached guest floor proximity is not an activation condition. Native Havok
  remains authoritative at airborne spawns and event-owned floors. A guest must
  support pose-only creation without terrain (verified for Mario).

Set `[Framework] StartingClass=0` before launching to disable the class replacement
while retaining the existing manual character toggle.

## Validation

Offline class-policy tests cover invalid profiles, manual override, reload and
recycled actor pointers. Real-adapter tests repeatedly create, animate, rotate
and destroy preview instances before running ordinary movement/combat tests.
Live verification and limitations are summarized in [Validation](VALIDATION.md).
