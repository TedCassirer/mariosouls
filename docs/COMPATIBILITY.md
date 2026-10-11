# Compatibility and support scope

MarioSouls targets **Windows Dark Souls: Prepare to Die Edition**, x86. Dark Souls
Remastered and other executables are unsupported. The accepted executable SHA-256
is recorded in [project metadata](../release/project.json); the installer verifies
it directly against that metadata.

Support is for **offline play**. Multiplayer compatibility has not been verified.
The mod does not provide a guarantee of online safety or a network sandbox.

The host uses `dinput8.dll`. A fresh installation or a recognized MarioSouls host
can be installed normally. An unknown existing loader is refused, including in
check-only mode. `--replace-loader` is an explicit override after reviewing that
conflict; it replaces the loader and keeps a backup. It does not chain two loaders
or make their mods compatible.

DSfix, other graphics wrappers and additional mods need a measured compatibility
matrix on a clean copy. Do not advertise compatibility merely because one session
started. Controller input uses the existing XInput integration; alternative
controllers and Steam Input profiles need their own checks.

Asset extraction uses local game files. Other game revisions, altered collision
or equipment data, and nonstandard installs may need additional preparation.
See [testing](TESTING.md) and [troubleshooting](TROUBLESHOOTING.md).
