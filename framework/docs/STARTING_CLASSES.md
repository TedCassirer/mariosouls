# Native starting classes

Optional `SoulsCharacterGetClassProfile` (version 1, `sdk/class_api.h`) lets an
adapter supply a class name, description and preview framing. The host maps the
generic firecaster archetype onto its native class, stats and casting equipment.
Adapters continue to have no memory or inventory access.

## PTDE implementation

- Pyromancer is native class byte 7 at PlayerGameData + C6. Native selection at
  FA4E50 writes that byte and requests preview initializer 3000 + class.
- CharaInitParam 2007 is the actual start; 3007 is the menu preview. These are
  separate 240-byte records. The original 2007 starts with a broken sword and
  attuned Fireball, but no Flame. At lookup, private copies change only the four
  hand equipment fields: Pyromancy Flame, empty secondary right slot, Plank
  Shield, empty secondary left slot. Native initialization owns inventory and
  spell allocation. All original attributes/armor/items remain intact.
- D1FA40 (EDI id, stack output, ret4) is signature checked. The host uses the
  existing type-checked parameter directory reader for slot 21. No archive or
  installed game-data binder is modified.
- D175E0 reads a single FMG entry (EDX id, stack FMG). The hook replaces only
  132027/132327 in the menu-other base/patch FMGs, identified through MsgRepository
  +138/+1F0. This covers both the direct menu-list lookup and the common wrapper.
- Creation is isolated from world ownership: MenuMan +94 values 2/3/5/7 (including name entry), absent
  native player, selected class 7, supported adapter, installed hooks. A temporary
  guest instance generates an idle mesh without gameplay ticks or controls. It
  is destroyed before world activation or when another class is selected.
- The full-body model panel is an indexed/triangle-fan SHORT2 quad with logical
  bounds (6600,1216)-(9568,4984), in eighth-pixels. Only its 512-square ARGB native
  texture is substituted. The existing UI still positions and clips the panel.
  GPU resources restore device state and are released on reset. Native face,
  hair and body options still describe the human used when the manual toggle is
  off; they do not change the guest mesh.
- Native saved class requests the adapter once per loaded actor. The manual
  toggle remains available and is not overwritten every frame. This replacement
  also applies to existing Pyromancer-class saves when loaded; no inventory is
  retroactively granted to them. Other classes retain their normal defaults.

Set `[Framework] StartingClass=0` before launching to disable the class replacement
while retaining the existing manual character toggle.

## Validation

Offline class-policy tests cover invalid profiles, manual override, reload and
recycled actor pointers. Real-adapter tests repeatedly create, animate, rotate
and destroy preview instances before running ordinary movement/combat tests.
Live verification and limitations are recorded in MODLOG.md after each test pass.
