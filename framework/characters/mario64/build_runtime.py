"""Build this adapter's pinned guest runtime; no ROM is downloaded."""

from pathlib import Path
import argparse
import subprocess
import sys
import shutil
from prepare_runtime import prepare
from runtime_geometry import prepare_geometry

root = Path(__file__).resolve().parents[3]
parser = argparse.ArgumentParser()
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
subprocess.run([sys.executable, root / "tools/dependencies.py"], check=True)
repo = root / "work/libsm64"
zig = root / "work/tools/zig-windows-x86_64-0.13.0/zig.exe"
staged = args.output / "runtime-source"
shutil.copytree(repo / "src", staged / "src", dirs_exist_ok=True)
prepare_geometry(root, staged)
repo = staged
prepare(repo)
dirs = [
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
sources = [str(p) for d in dirs for p in sorted((repo / d).glob("*.c"))]
sources.append(str(Path(__file__).with_name("runtime_body.c")))
cmd = [
    str(zig),
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
    str(repo / "src/decomp/include"),
    "-I",
    str(repo / "src"),
    "-shared",
    *sources,
    "-o",
    str(args.output / "sm64.dll"),
    "-lm",
]
log = args.output / "runtime.build.log"
print("Building Mario runtime from pinned sources", flush=True)
with log.open("w") as stream:
    result = subprocess.run(cmd, stdout=stream, stderr=subprocess.STDOUT)
if result.returncode:
    print(log.read_text()[-10000:])
    sys.exit(result.returncode)
print("Built " + str(args.output / "sm64.dll"))
