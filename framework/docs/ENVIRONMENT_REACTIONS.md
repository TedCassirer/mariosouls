# Native damage, lava and fire reactions

Native Dark Souls HP and protection remain authoritative. This integration observes native effects/hits and requests guest presentation/movement; it does not add a second damage pool or copy guest invulnerability flags into the game.

Live lava/ring, real firebomb and mixed-damage coverage remains incomplete. Synthetic packets and animation screenshots are not evidence that a real hazard's damage/protection works end to end.

## Lava

Unprotected grounded lava contact requests SM64's burning-butt bounce, including launch, sound, air control and landing recovery. It does not restart every render frame.

The Orange Charred Ring in either native ring slot suppresses that bounce. Equipping it during the reaction cancels the lava action without removing valid airborne movement. Native lava damage and ring mitigation remain native; the feature does not invent an immunity percentage. The ring does not suppress reactions to enemy fire.

## Enemy fire and recovery

A native fire-bearing hit must actually lower HP before requesting burning run, or burning fall while airborne. It has a **10-second start-to-start cooldown**. Further hits still deal native damage, without restarting/extending the burning action during cooldown. Ordinary interactions/hitstun do not clear the cooldown; a new guest instance/life resets it.

Burning takes precedence over ordinary flinch/knockdown through verified safe native completion, retaining the guest burning action at handback. Death, paired attacks, grabs, menus and protected scripts retain priority. A confirmed fire event may wait up to five seconds for eligible control; expiry, actor/map replacement or teardown discards it.

No extra damage-over-time, stamina cost or protection is added. Fully blocked/no-HP-loss hits do not ignite Mario. Mixed physical/fire packets establish a damaging fire-bearing hit, not a separately measured post-defense fire-only amount.

The guest action/audio is supplied; the SM64 particle scheduler is not imported as a complete native flame-effect renderer.

## PTDE observations

Owned-parameter inspection identifies terrain 27 using effects 4030/4031 (state 142), and ring 139 using effect 2210 (state 186). The remaining lava effect means the ring must not be documented as blanket immunity.

`native_environment.h` mirrors the active-effect traversal inspected at DC02C0: character +1E0 to manager +4/node list, mask 800C0003, parameter bytes +2C, state +156 and next +30. It rejects unreadable/cyclic/overlong lists and gates observation on executable instructions.

`fire_hooks.h` wraps E6BFE0 around original damage application (target in EAX, three stack arguments, callee-pop 12). Packet +8 is fire attack power and +16C resolved total HP damage; actual before/after native HP loss is required. The source may be a projectile/explosion without a readable character owner. Explicit self hits are excluded, without inventing a team classification from an untyped pointer.

Hooks preserve registers, flags, x87, SSE and MXCSR. Missing observation support keeps native damage behavior and skips that guest extension. These are PTDE facts to rediscover for Remastered, not portable addresses.

## Verification and updates

Use a disposable character to compare native/Mario lava damage with both ring slots, unequip/equip during a reaction, healing, death and kill planes. Check real enemy fire, airborne hits, cooldown, ordinary-hit priority and native ownership interruptions.

Rebuild host/adapter/runtime together for integration changes. Record live evidence separately from compilation and update [Features and tweaks](../../docs/FEATURES_AND_TWEAKS.md), [CHANGELOG.md](../../CHANGELOG.md), the [SDK contract](CONTRACT.md) and [PTDE inventory](../hosts/ds1ptde/PORT_CHECKLIST.md) where affected.
