# Firelink cannon and Bob-omb Buddy

The optional launcher capability provides one cannon/operator pair near Firelink's central bonfire in Mario mode. The host places the objects and bridges input/camera/native motion; the guest owns cannon actions and ROM-derived visuals.

## Controls

1. Stand near the Buddy and press A/Q to unlock.
2. Approach the cannon and press A/Q to enter; opening takes about five seconds.
3. Aim with either stick or WASD. The right stick wins if both move.
4. Release entry, then press A/Space to fire. Y/Q leaves the cannon.

Holding entry cannot immediately fire. Y is cancellation only; it cannot talk to the Buddy, enter the cannon or activate native world prompts. Native prompts take priority over custom objects. Ordinary camera input/lock-on/mouse rotation are suppressed during aiming.

## Guest and host work

The runtime uses libsm64 cannon entry/flight, sounds, aim limits, muzzle displacement, velocity and gravity at 30 Hz. Missing opening actions are staged from pinned SM64 source. Cannon/Buddy meshes, textures and joint channels are decoded from the owned US ROM.

This is not the full SM64 object scheduler, Buddy dialogue/AI or save system. The operator uses proximity interaction, faces the view and plays its decoded idle animation.

The host supplies a circular sight and fixed cannon-eye camera pivot using the same yaw/pitch as launch. Camera ownership is actor/camera-bound, expires after 150 ms without renewal and does not overwrite the normal native simulation camera. Damage, death, focus loss, native takeover and actor/map changes cancel opening/aiming.

Launches use ordinary native collision/health and Mario's landing policy. There is no cannon invulnerability or guaranteed safe landing.

## Placement and preparation

`[WorldObjects.FirelinkLauncher]` in `mariosouls.ini` accepts metre-space `X`, `Y`, `Z`, radian `Yaw` and `Unlocked`. Defaults are `(-45, -60.5, 56)`, yaw `-pi/2`; Buddy starts 2.5 m east. Both snap to verified floor within 2 m, otherwise the kit is suppressed.

Unlock state is local mod configuration shared between saves, not a native quest flag. Only the Firelink placement is implemented; the earlier Asylum QA placement is gone.

Build the runtime with `build --with-runtime` / `build --check-portability`. Local collision preparation keeps event-controlled barriers native-owned. ROMs, generated meshes/caches, binary outputs and captures remain outside Git.

## Verification

Check unlock, entry/opening, fresh-button launch, multiple aim directions, pivot, landing/control return and cancellation. Use [manual verification](../../docs/TESTING.md) rather than assuming the guest cannon API proves native integration. Update [Features and tweaks](../../docs/FEATURES_AND_TWEAKS.md) and the [SDK contract](CONTRACT.md) when semantics/tuning change.
