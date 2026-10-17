# Collision and combat overlay

The overlay is a bounded diagnostic view of the framework's current observations. It does not edit the world, enable cheats or expose every native collision shape.

## Controls

| Key | Action |
| --- | --- |
| F8 | Toggle overlay visibility; starts hidden |
| F9 | Cycle Combat, Terrain, Objects and All |
| F10 | Toggle through-walls drawing |
| F6 | Independently toggle Mario/native mode |

Keys are focus-gated and edge-triggered. A held key on focus return must be released before it triggers. The overlay also works with native controls for comparisons and hotkeys do not save configuration. Removed F1..F4/F7/F11 developer functions have no replacement.

## What the colors mean

| Display | Observation |
| --- | --- |
| Cyan capsule | Live native player movement capsule |
| Pink capsule | Validated secondary body-contact capsule |
| Red / purple capsules | Nearby living hostile / other actor movement capsules |
| Amber volume | Guest attack range and height limits |
| Green samples | Bounded points accepted by the read-only guest contact predicate |
| White cross | Contact followed by observed native HP loss through feedback correlation |
| Small amber cross | Contact submitted, without proof of damage |
| Yellow contact/normal | Live native movement contact while guest locomotion is owned |
| Teal triangles | Nearby guest collision-cache geometry |
| Orange marker | Native object origin, not an object bounding box |

Recent attack geometry persists for 650 ms and contact crosses for 1.5 seconds. Actor/map changes clear stale observations. An exhausted attack does not get an active sample volume. Confirmed-hit crosses require the feedback system to be enabled.

## Limits

Body capsules are not per-bone hurtboxes. Enemy weapon hitboxes, arbitrary object bounds and complete dynamic meshes are not mapped. Terrain is the guest cache, not the entire live Havok world; omitted event barriers can still have native collision. Missing triangles are not evidence that native collision should be deleted.

The sample grid approximates eligibility. Facing, occlusion, stamina, target limits, native defenses/parts and immunity can still reject damage. Sampling never calls a contact commit or a simulation step.

Rendering uses the pre-HUD boundary, preserves device state and does not write depth. It adds no native gameplay-memory writes. View toggles can be logged; no periodic geometry/movement journal is recorded.

## Startup settings

```ini
[Debug]
Layer = 0
ThroughWalls = 1
RadiusMeters = 12
MaxTerrainTriangles = 3000
```

Layer 0/1/2/3 selects Combat/Terrain/Objects/All. Radius clamps to 4..40 m and terrain to 100..5000 triangles. Terrain refreshes at most four times per second and reports CAPPED when truncated. Visibility always starts off rather than inheriting an old enabled setting.

Use this view with the [bug-report guide](../../docs/BUG_REPORTING.md). [DS Gadget](https://github.com/JKAnderson/DS-Gadget) and [DS Anim Studio](https://github.com/Meowmaritus/DSAnimStudio) are separate tools; they are not bundled and do not understand the guest attack predicate. Update [Features and tweaks](../../docs/FEATURES_AND_TWEAKS.md) if controls or displayed semantics change.
