"""Check local links in the maintained contributor and player documentation."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def check(root=ROOT):
    files = [
        root / name
        for name in (
            "README.md",
            "CONTRIBUTING.md",
            "THIRD_PARTY.md",
            "CHANGELOG.md",
            "SECURITY.md",
            "framework/README.md",
        )
    ]
    files += sorted((root / "docs").glob("*.md"))
    files += sorted((root / "framework/docs").glob("*.md"))
    files += sorted((root / "framework/hosts").glob("*/*.md"))
    files += sorted((root / "framework/characters").glob("*/*.md"))
    files += [
        root / "framework/hosts/ds1ptde/qa/README.md",
        root / "tools/README.md",
        root / "tools/tests/README.md",
    ]
    failures = []
    for path in files:
        source = path.read_text(encoding="utf-8")
        for link in re.findall(r"\[[^\]]*\]\(([^)]+)\)", source):
            target = link.split("#", 1)[0]
            if not target or re.match(r"[a-zA-Z]+:", target):
                continue
            if not (path.parent / target).exists():
                failures.append(str(path.relative_to(root)) + ": " + target)
    if failures:
        raise RuntimeError("Broken documentation links:\n" + "\n".join(failures))
    print("Maintained documentation links passed (" + str(len(files)) + " files).")


if __name__ == "__main__":
    check()
