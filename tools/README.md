# Developer tools

Run commands from the repository root. `framework.py` is the supported entry
point for building, testing, installing and scaffolding adapters; its `--help`
lists the available commands.

| Module | Responsibility |
| --- | --- |
| `framework.py` | Command dispatch, compiler invocation, build identity and receipts |
| `profiles.py` | Character manifest validation and generated SDK descriptors |
| `framework_tests.py` | Offline test orchestration and regression receipts |
| `results.py` | Per-check logs, JSON results and JUnit reports |
| `framework_install.py` / `framework_uninstall.py` | Validated installation transactions and restoration |
| `dependencies.py` | Fetch and verify pinned dependencies and the compiler |
| `release.py` | Source audit, package construction and package validation |
| `check_docs.py` | Maintained documentation link checks |
| `tests/` | Isolated tests for these tools, using temporary fixtures |

The internal modules support the entry point; they are not additional competing
build workflows. Character asset recipes stay with their adapters under
`framework/characters/`. Host asset recipes stay under `framework/hosts/`.
Live-game checks are separate from offline tests; see
[the QA guide](../framework/qa/README.md).
`python tools/framework.py qa` lists the maintained live scenarios without
opening the game; append a scenario and `--help` to inspect its arguments.

Install `requirements-dev.txt` for source formatting, then use:

```powershell
python tools/framework.py format --check
python tools/framework.py test --shared-only
```

See [Development](../docs/DEVELOPMENT.md) for dependency setup and the ROM-backed
suite. Local scratch tools, captures and investigation results belong in ignored
`work/`, rather than becoming new top-level commands.
