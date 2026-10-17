# Security and private reports

Only the current development branch is maintained, with no guaranteed security-response time. Report ordinary gameplay/crash problems through the [bug-report guide](docs/BUG_REPORTING.md).

For a vulnerability involving user data or unintended code execution, contact the maintainer privately before publishing details. Do not include secrets or an exploit against another user's installation in a public issue.

## Review shared material

Logs/captures can reveal local paths and player names. Remove them before posting. Do not upload ROMs, saves, original/extracted game data, memory dumps, account tokens, private keys or complete recovery backups.

## Trust boundaries

MarioSouls and its adapters are native code loaded inside the game. Executable fingerprints, signature/pointer/buffer checks, pinned hashes and file-ownership validation reduce accidental incompatibility; they are not a security sandbox or publisher authentication.

The local installer is readable PowerShell/Python source. Review the source and origin of anything you run. It does not download a ROM, alter the machine execution policy, elevate itself or authorize replacement of an unknown loader. See [installation](docs/INSTALLING.md) for its actual behavior.
