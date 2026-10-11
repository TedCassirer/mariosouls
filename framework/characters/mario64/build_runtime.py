"""Build this adapter's pinned guest runtime; no ROM is downloaded."""

from pathlib import Path
import argparse
import subprocess
import sys
import shutil
from prepare_runtime import prepare
from runtime_geometry import prepare_geometry

ROOT = Path(__file__).resolve().parents[3]
SOURCE_DIRECTORIES = [
    "src",
    "src/decomp",
    "src/decomp/engine",
    "src/decomp/include/PR",
    "src/decomp/game",
    "src/decomp/pc",
    "src/decomp/pc/audio",
    "src/decomp/mario",
    "src/decomp/tools",
    "src/decomp/audio",
]


def compiler_command(source_tree, output, toolchain):
    """Construct the pinned compiler invocation from the staged source tree."""
    sources = [str(p) for d in SOURCE_DIRECTORIES for p in sorted((source_tree / d).glob("*.c"))]
    sources.append(str(Path(__file__).with_name("runtime_body.c")))
    command = [
        str(toolchain),
        "cc",
        "-target",
        "x86-windows-gnu",
        "-std=gnu11",
        "-O2",
        "-fno-strict-aliasing",
        "-DSM64_LIB_EXPORT",
        "-DGBI_FLOATS",
        "-DVERSION_US",
        "-DNO_SEGMENTED_MEMORY",
        "-Wno-int-conversion",
        "-Wno-incompatible-pointer-types",
        "-I",
        str(source_tree / "src/decomp/include"),
        "-I",
        str(source_tree / "src"),
        "-shared",
        *sources,
        "-o",
        str(output / "sm64.dll"),
        "-lm",
    ]
    return command


def build_runtime(output):
    """Stage private sources and build the runtime, leaving the checkout untouched."""
    output.mkdir(parents=True, exist_ok=True)
    subprocess.run([sys.executable, ROOT / "tools/dependencies.py"], check=True)
    checkout = ROOT / "work/libsm64"
    toolchain = ROOT / "work/tools/zig-windows-x86_64-0.13.0/zig.exe"
    staged = output / "runtime-source"
    shutil.copytree(checkout / "src", staged / "src", dirs_exist_ok=True)
    prepare_geometry(ROOT, staged)
    prepare(staged)
    command = compiler_command(staged, output, toolchain)
    log = output / "runtime.build.log"
    print("Building Mario runtime from pinned sources", flush=True)
    with log.open("w") as stream:
        result = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode:
        print(log.read_text()[-10000:])
        return result.returncode
    print("Built " + str(output / "sm64.dll"))
    return 0


def main(arguments=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(arguments)
    return build_runtime(args.output)


if __name__ == "__main__":
    sys.exit(main())
