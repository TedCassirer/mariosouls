# Mario / Dark Souls PTDE bridge contract

Native x86 DLL; libsm64 fd11813208272b4271d92bd92feb8f3fdbe61be5 is stepped at 30 Hz. Dark Souls remains the host of rendering, camera, health, world progression, NPCs, doors and saves. Mario simulation owns position only while enabled. Initial controls: WASD movement, Space jump, left mouse punch/kick, Ctrl crouch/ground pound, F6 toggle. Native control returns when disabled or during loading/death.

All game addresses must come from unique code signatures, with nullable pointer chains. No remastered offsets. Memory must be readable/writable before access. Unknown executable fingerprint refuses activation.

World bridge initially uses 100 Mario units per game metre, Y up, with axis/winding adaptation to be measured in-game. Private collision cache MSC1 is little-endian: four-byte magic, uint32 triangle count, then float32[triangle][vertex][xyz]. It comes only from the owner's game archives and is not distributable mod content. Static maps first; moving objects require a separate bridge.

Draw must restore all D3D9 state, draw into a scene target using its depth buffer, and survive reset. Render thread owns libsm64 calls. Limit catch-up to three steps; release inputs on focus loss. Invalid outputs suspend movement; stale pointers or loading clear the active player. Never create damage before combat mapping is tested. Distinguish rendered Mario, working movement, and connected attacks in reports.

ROM remains in its original local path. Match 8 MiB US ROM SHA-1 9bef1128717f958171a4afac3ed78ee2bb4e86ce before use. Neither ROM nor textures are included in releases.
