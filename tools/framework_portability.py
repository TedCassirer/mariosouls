"""Compile portable headers and guest binaries; never execute checks or build a DSR host."""

import sys
import framework as f


def check(character):
    folder, profile = f.load_profile(character)
    for target in ("x86-windows-gnu", "x86_64-windows-gnu"):
        out = f.BUILD / "portability" / target
        f.compile_cpp(
            f.FRAMEWORK / "build/portable_headers.cpp",
            out / "portable-headers.o",
            extra=["-c"],
            target=target,
        )
        out.mkdir(parents=True, exist_ok=True)
        log = out / "sdk-c.build.log"
        with log.open("w") as stream:
            f.run(
                [
                    f.ZIG,
                    "cc",
                    "-target",
                    target,
                    "-std=c11",
                    "-Werror=incompatible-pointer-types",
                    "-c",
                    f.FRAMEWORK / "build/sdk_headers.c",
                    "-o",
                    out / "sdk-c.o",
                ],
                stdout=stream,
                stderr=f.subprocess.STDOUT,
            )
        if target == "x86_64-windows-gnu":
            f.profile_header(profile, out / "profile.generated.h")
            f.compile_cpp(
                folder / profile["source"],
                out / (character + ".dll"),
                [out],
                ["-shared"],
                target=target,
            )
            if profile["assets"].get("runtime"):
                f.run(
                    [
                        sys.executable,
                        folder / "build_runtime.py",
                        "--target",
                        target,
                        "--output",
                        out / "runtime",
                    ]
                )
    print(
        "Compiled C/C++ SDK and shared headers for x86/x64, plus x64 guest binaries. No binaries executed; PTDE remains x86 only."
    )
