"""Portable policy/ABI validation. Does not build a 64-bit PTDE host."""

import framework as f

POLICY_TESTS = [
    "portability",
    "timing",
    "simulated_host",
    "core",
    "class",
    "motion",
    "portable_policy",
    "feedback",
    "kinematic",
    "contact",
    "debug_geometry",
]


def check():
    for target in ("x86-windows-gnu", "x86_64-windows-gnu"):
        out = f.BUILD / "portability" / target
        for name in POLICY_TESTS:
            exe = out / (name + ".exe")
            f.compile_cpp(f.FRAMEWORK / "tests" / (name + "_test.cpp"), exe, target=target)
            f.run([exe])
        c_exe = out / "sdk-c.exe"
        f.run(
            [
                f.ZIG,
                "cc",
                "-target",
                target,
                "-std=c11",
                f.FRAMEWORK / "tests/sdk_c_test.c",
                "-o",
                c_exe,
            ]
        )
        f.run([c_exe])
        # Compile both implemented adapters, then exercise a generated adapter
        # through the public DLL contract, with no copyrighted assets required.
        for character in ("mario64", "link_ww"):
            folder, profile = f.load_profile(character)
            f.profile_header(profile, out / "profile.generated.h")
            f.compile_cpp(
                folder / profile["source"],
                out / (character + ".dll"),
                [out],
                ["-shared"],
                target=target,
            )
        runner = out / "adapter-test.exe"
        f.compile_cpp(f.FRAMEWORK / "tests/adapter_test.cpp", runner, target=target)
        generated = f.BUILD / "scaffold-test/characters/sdk_probe"
        f.compile_cpp(
            generated / "adapter.cpp", out / "probe.dll", [generated], ["-shared"], target=target
        )
        f.run([runner, out / "probe.dll"])
    print(
        "PASS portable policies, C/C++ ABI and adapter DLL validation: x86 + x64 (no Remastered host)"
    )


def check_owned_character(character, asset, runtime):
    """Exercise owned assets in x64 guest DLLs; never build/install a native host."""
    import json

    target = "x86_64-windows-gnu"
    folder, profile = f.load_profile(character)
    if not asset or (profile["assets"].get("runtime") and not runtime):
        raise ValueError("Supply owned --asset and, when required, a 64-bit --runtime")
    out = f.BUILD / "portability" / target / "owned" / profile["id"]
    out.mkdir(parents=True, exist_ok=True)
    report = out / "report.json"
    report.unlink(missing_ok=True)
    f.profile_header(profile, out / "profile.generated.h")
    dll = out / (character + ".dll")
    f.compile_cpp(folder / profile["source"], dll, [out], ["-shared"], target=target)
    runner = out / "adapter-test.exe"
    f.compile_cpp(f.FRAMEWORK / "tests/adapter_test.cpp", runner, target=target)
    f.run([runner, dll, asset, runtime or ""])
    checks = ["adapter"]
    for name in ("test", "slope_test", "death_test", "equipment_test", "lifecycle_test"):
        source = folder / (name + ".cpp")
        if not source.is_file():
            continue
        exe = out / (character + "-" + name + ".exe")
        f.compile_cpp(source, exe, target=target)
        args = [exe, dll, asset]
        if name != "test":
            args.append(runtime or "")
        f.run(args)
        checks.append(name)
    report.write_text(
        json.dumps(
            {
                "status": "passed",
                "target": target,
                "character": character,
                "checks": checks,
                "scope": "owned-asset-adapter-only",
                "authorizes_install": False,
                "adapter_sha256": f.sha(dll),
                "asset_sha256": f.sha(f.Path(asset)),
                "runtime_sha256": f.sha(f.Path(runtime)) if runtime else None,
            },
            indent=2,
        )
    )
    print("PASS x64 owned-asset adapter checks. No native host compatibility is implied.")
