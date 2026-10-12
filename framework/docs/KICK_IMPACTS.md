# Poise-aware kick impacts

The shared impact policy requests native knockback for selected guest attacks.
The host applies that request through its private attack rows and damage packet;
it does not move enemies by writing their positions or velocities. Native
reaction rules, defenses and collision determine the resulting travel.

## Mario tuning

| Move | Poise damage | Maximum requested push |
| --- | ---: | ---: |
| Combo-finishing kick | 30 | 1.40 m |
| Jump kick | 25 | 1.15 m |
| Slide kick | 35 | 1.60 m |

Requested distance decreases linearly with maximum poise, reaching zero at 80.
Current poise must be below the attack's poise damage before a push is requested.
A capsule radius above 0.80 m or total height above 2.80 m blocks added push even
when the creature's numeric poise is zero. Boss resistance can also come from
native effects, rather than the reported poise value alone.

Each enemy can receive an enhanced push request at most once per 1.2 seconds.
Punches do not spend this cooldown. Stamina costs, HP attack power and
one-hit-per-attack limits remain governed by the shared combat policy.
Requested metres are tuning values, not guaranteed displacement.

## Implementation

- The optional SDK impact profile lets an adapter select attacks and distances.
- `core/impact.h` validates the profile, scales distance and tracks cooldowns.
- `hosts/ds1ptde/native_combat.h` supplies the native packet path; the host reads
  current/maximum poise and capsule bounds before requesting a push.
- Native HP, poise, guard, reaction animations, AI, deaths and rewards remain
  authoritative. A knockback request does not force a stagger animation.

See the [SDK contract](CONTRACT.md) for profile ownership and versioning.

## Verification and limits

Offline checks cover invalid profiles, poise thresholds, resistance scaling,
large bodies, cooldown reuse and clock wrap. Real-adapter checks reject
incompatible profile versions and sizes.

Historical Mario QA checks observed finishing-kick and slide-kick displacement
of about 1.67 m and 1.82 m on an Asylum hollow, and about 0.55 m for an isolated
jump kick. The hollow recovered and resumed its AI. The Asylum Demon's large
capsule suppressed added push despite its reported zero poise. These results
belong to the tested historical build, not every subsequent revision.

Intermediate/high-poise enemies, other bosses, all wall/ledge cases and every
defensive special effect have not been verified live. See the
[archived measurements and test conditions](https://github.com/TedCassirer/mariosouls/blob/53a8e61/framework/docs/KICK_IMPACTS.md)
and the [current coverage guide](VALIDATION.md). The maintained
[kick QA scenario](../hosts/ds1ptde/qa/README.md) records native responses on a guarded fixture.
