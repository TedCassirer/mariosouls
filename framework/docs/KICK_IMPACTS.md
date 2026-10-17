# Poise-aware kick push

The impact profile requests native push for selected guest attacks. The host supplies a scoped value in private attack rows/native damage packets; it does not move enemies by writing their position or velocity.

## Mario defaults

| Attack | Poise damage | Maximum push request |
| --- | ---: | ---: |
| Finishing kick | 30 | 1.40 m |
| Jump kick | 25 | 1.15 m |
| Slide kick | 35 | 1.60 m |

Requested distance decreases linearly with maximum poise and reaches zero at 80. Current poise must be below the attack's poise damage. Radius above 0.80 m or total height above 2.80 m excludes the added push, even if reported poise is zero. Native boss effects/guard/reaction rules can resist independently of these observations.

Each target has a 1.2-second enhanced-push cooldown; punches do not spend it. The shared attack window still controls stamina, HP power, target limits and deduplication. Requested metres are tuning values, not guaranteed travel, and do not imply every boss has a special blanket flag.

## Boundaries and verification

The adapter's optional impact profile selects attacks/distances. `core/impact.h` validates/scales requests and tracks cooldowns; `hosts/ds1ptde/native_combat.h` routes them through native construction. Native defenses, poise, collision, AI, reaction animations, deaths and rewards decide the result.

Compare a hollow, high-poise enemy and large boss, including repeated kicks, wall contact and ledges. Measure actual stagger/travel; neither a profile value nor compilation proves displacement. No behavioral fixture or live automation runner is maintained.

Keep the [SDK contract](CONTRACT.md), [feature inventory](../../docs/FEATURES_AND_TWEAKS.md) and changelog current when these values or rules change.
