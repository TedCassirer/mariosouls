# Compatibility and evidence

## Supported implementation

MarioSouls targets **Dark Souls: Prepare to Die Edition on Windows/x86**. The native executable fingerprint is defined in [edition.json](../framework/hosts/ds1ptde/edition.json). Other executables are refused rather than matched by guessed addresses.

Mario is the only implemented adapter. Remastered has no implemented hooks, DX11 renderer or playable compatibility. The code is structured to prepare that future work; see [host porting](../framework/docs/HOST_PORTING.md).

## Rendering checks

| Configuration | Evidence and scope |
| --- | --- |
| Stock PTDE without DSfix | Focused historical launch/rendering checks |
| DSfix 2.4, 30 FPS | Focused historical launches/movement at 1080p and a 1440p scene downsampled to 1080p UI |
| DSfix at 60 FPS | Visual interpolation is implemented and used during development; native timing quirks and full-game behavior still need manual coverage |
| Recent cleanup/revisions | Compile/source/file checks do not repeat all historical game checks |

The host observes the real native D3D9 device, supports separate scene/UI sizes and draws Mario before native text/menus. It does not rely on a dummy device or private DSfix object layouts.

The installer recognizes the locally verified DSfix 2.4 DLL SHA-256:

```text
c6855c833763da5872fc2feebfc9e2a87d0c3332b11485d535267bfc14f10e0d
```

DSfix is supplied separately. With it, keep its `dinput8.dll` and chain `MarioSouls-host.dll`; without it the host uses `dinput8.dll` directly. [Installation](INSTALLING.md) handles this arrangement. Disable native anti-aliasing/motion blur for DSfix. The installer preserves the current FPS limit and other graphics settings.

## Gameplay and other mods

Native health, stamina, armor, status effects, inventory and rewards remain in the native systems. That architecture is not evidence that every status, grab, boss or scripted event has been exercised. The [feature inventory](FEATURES_AND_TWEAKS.md) marks important implementation/verification limits, including lava, fire reactions, cutscenes and traversal.

Controllers, Steam Input mappings, custom HUDs, texture replacements, alternate wrappers and other native hooks can conflict and need individual checks. Unsupported loaders/wrapper chains are not overwritten by setup. Multiplayer compatibility and online safety are unverified; development is intended for offline play.

Use owned US SM64 data and supported PTDE exports. Collision/equipment data is edition-specific and cannot establish Remastered compatibility. See [manual verification](TESTING.md) when adding a checked configuration; record the exact build, settings, area and result.
