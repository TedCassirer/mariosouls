# Developer tool tests

These isolated tests exercise installation transactions, package validation,
diagnostic serialization, manifest boundaries, build identity and test-result reporting. Fixtures use
temporary directories; they do not touch the installed game or player saves.

Run the supported suite from the repository root:

```powershell
python tools/framework.py test --shared-only
```

After the diagnostic fixture has been compiled by that suite, individual Python
modules can also be run with `python tools/tests test_framework_install` or
`python tools/tests test_diagnostics`. Gameplay and SDK tests live under
`framework/tests/`; character-specific tests stay beside their adapters.
