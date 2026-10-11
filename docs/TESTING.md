# Testing

## Shared CI

```powershell
python tools/dependencies.py --setup --download-toolchain
python tools/framework.py test --shared-only
python tools/framework.py build --with-runtime
```

Windows CI runs these without ROMs, saves or an installed game. Checks cover core
policies, input mapping, hook contracts, collision filters, equipment formats,
SDK scaffolding, install/rollback transactions and package integrity. The build
compiles the host, Mario adapter and runtime. It does not run gameplay.

`shared-test-report.json` is separate from the full test receipt and explicitly
cannot authorize installation. Rendering is skipped on hosted CI; `--render`
adds the D3D9 check on a local machine with a graphics device.

Each shared check has a named result, duration and separate output log under
`work/framework-build/test-results/`. `results.json` records suite status;
`junit.xml` provides machine-readable results, including early build failures.
CI uploads these alongside compiler logs even on failure. They describe offline
checks, not live gameplay coverage.

The [bug reporter](BUG_REPORTING.md) adds regression fixtures for transitions,
health changes and unavailable positions, plus bounded histories, clock wrap,
privacy fields, HTML escaping, report-file failures and build identity.

## Full local tests

Follow [Development](DEVELOPMENT.md) for the ROM-backed command. The suite includes
rendering, the real Mario runtime and character-specific actions. It records the
exact binaries and asset hash. Passing tests does not prove all Dark Souls areas
work. Install checks require this full receipt, not just shared CI.

## Live validation

Use backed-up disposable characters such as Mario QA. Preserve the existing
character and level guard; never use the owner's level-65 character. Obtain a
current control handoff before automated input. Do not leave invincibility or
QA controls enabled after a test.

Record build hash, character, map, equipment, steps, expected result, actual
result, enemy HP changes, stamina, timing and stagger where applicable. Keep
private captures and saves out of Git. Add concise verified findings to the
[validation log](../framework/docs/VALIDATION.md).

The remaining release matrix includes:

- Fresh load, menu return, death/respawn and area transitions while running.
- Doors, NPCs, bonfires, bloodstains, healing, ladders, lifts and removable floors.
- Enemy melee/projectiles, hazards, lethal pits and ordinary landings.
- Unarmed attacks, weapon families, shields, parry/criticals and fireballs.
- Physical controller and keyboard/mouse, supported display resolutions.
- Fresh install, repeat install, loader conflict refusal, uninstall and recovery.

A useful result distinguishes passed, failed and untested cases. Do not infer
live success from a contract test or an animation alone.
