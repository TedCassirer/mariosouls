# Dark Souls PTDE Mario integration

- Host: Steam Dark Souls Prepare to Die Edition, app 211420/build 1480646, Windows. Native executable and D3D9 route to be confirmed from PE.
- Goal: Mario 64 movement, animated Mario model, Mario attacks affecting Dark Souls enemies.
- Route: libsm64 movement/animation library embedded in a PTDE-specific native plugin, with the existing native player retained for game systems.
- Required user asset: own US Super Mario 64 ROM. Pending.
- First vertical slice: plugin loads, reads the native player position and camera safely, and renders a depth-tested marker at the player in a real scene. Then feed real collision to libsm64 and replace movement; integrate attacks last.
- Save protection: verified snapshot of both discovered save locations before any game launch. No automatic progression save editing.
- Unknowns: exact player/camera/Havok layouts for this executable; correct game-thread update hook; render pass; collision extraction; native damage entry point; input device.
- Status: reconnaissance; no playable build, no install yet.
