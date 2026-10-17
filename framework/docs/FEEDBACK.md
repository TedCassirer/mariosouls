# Confirmed-hit feedback

Impact feedback makes accepted attacks readable without replacing native damage. The host first submits through the native attack builder, then correlates the target identity/contact with an observed HP drop within 180 ms. A native function's return value is not interpreted as damage.

Missing/recycled targets, healing, expired correlation and unchanged HP do not produce a confirmed hit. Object HP follows the same rule. Simultaneous damage from another source to the same target within the short receipt window cannot be perfectly disambiguated.

## Response

Weight comes from the attack multiplier, not final HP loss or character stats. A 0.65 multiplier gives the light end of the response; 1.65 or above gives the heavy end.

| Effect | Current range |
| --- | --- |
| Guest attacker hit stop | 34..67 ms, sampled by the 30-Hz simulation |
| Procedural sound | Approximately 130 ms thump/crack |
| World-space burst | 180..230 ms |
| Controller rumble | 80..135 ms |

Native simulation, enemies, health and collision reconciliation continue during guest hit stop. Incoming damage cancels the pause; paused time is discarded rather than replayed later. Jump/attack/crouch taps are buffered once with 150-ms expiry, and focus/lifecycle/native-action boundaries clear them.

Sound/pause/rumble are limited per attack serial; multiple accepted targets can receive bounded bursts. The host maintains at most eight bursts and 64 pending damage receipts. Rumble mixes with native vibration using the stronger motor values and expires independently of rendering. Visual bursts draw in world space with depth, before native UI.

## Configuration

```ini
[Feedback]
Enabled = 1
HitStop = 1
Sound = 1
Flash = 1
Rumble = 1
Volume = 65
RumbleStrength = 75
```

These settings reload about every two seconds. Switches use 0/1; volume/rumble clamp to 0..100. Effect durations stay bounded by shared policy. Disabling hit stop does not disable native damage.

Native HP/stamina, defenses, poise and rewards are unchanged. Fully blocked/immune contacts do not invent flesh-impact feedback. Listen/feel and compare hits, misses, blocks and area attacks in the [manual checklist](../../docs/TESTING.md); successful API calls alone are insufficient evidence. Keep changes in the shared feedback boundary and update [Features and tweaks](../../docs/FEATURES_AND_TWEAKS.md).
