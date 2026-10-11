# Shared impact feedback

The DS1 host emits feedback only after its private native attack packet was built
and it observes that target's HP decrease within 180 ms. The native function's
return value is not damage. Receipts include the target pointer, identity, attack
serial, starting HP and contact position. Missing/recycled targets, healing,
expired receipts and unchanged HP do not produce a damage effect. Object HP uses
the same policy. This is a bounded correlation with a native damage submission;
it cannot disambiguate simultaneous damage from another native source to that
same target within the receipt window.

The shared core derives impact weight from the attack descriptor's multiplier:
0.65 gives a light response, 1.65 or higher a heavy response. This deliberately
does not scale with stats or final HP loss: a light punch remains a light punch
against a weak enemy. The SDK and character movement constants are unchanged.

- Attacker-only hit stop: 34–67 ms, sampled by the existing 30 Hz simulation.
  The world, enemies, native health, collision reconciliation and input polling
  keep running. No native animation, invulnerability, damage or stagger values
  are changed. Incoming damage cancels the pause. Paused time is discarded rather
  than simulated in a later catch-up burst.
- Jump, attack and crouch taps during the pause are buffered once, with a 150 ms
  expiry. Focus loss, death, interaction entry and character release clear them.
- A 130 ms original procedural sound combines a low thump with a short crack.
  It uses a bounded, independent audio channel and no borrowed audio assets.
- A small expanding gold/white contact burst lasts 180–230 ms. It renders in
  world space with depth testing, below native subtitles, prompts and menus.
- An 80–135 ms XInput pulse mixes with native vibration using the stronger value
  on each motor. A separate expiry timer restores native vibration even if
  rendering stalls; focus loss stops the pulse.
- One sound, pause and rumble per attack serial, including area attacks. Each
  damaged target may get one burst, with at most eight bursts and 64 receipts.

Native HP/stamina, defenses, poise, deaths and rewards stay authoritative. Fully
blocked or invulnerable contacts do not produce the added flesh-impact feedback;
this feature does not invent a separate block detector or override guard effects.

## Settings

The installer adds missing defaults and preserves existing choices in
`DATA/mariosouls.ini`. Settings reload while playing, about every two seconds:

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

Switches accept 0/1. Volume and RumbleStrength are clamped to 0–100. Hit-stop
durations are intentionally bounded in the shared policy, not unrestricted INI
values. Disable HitStop independently if the extra pause is not preferred.

`Impact confirmed` / harness `impact_confirmed` records the actual before/after
HP, attack serial, confirmation latency and requested stop duration. Audio and
renderer submission results are logged separately; API success alone is not proof
that a person heard the sound or felt a physical controller.
