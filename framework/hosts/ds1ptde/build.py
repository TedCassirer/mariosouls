"""Build the implemented PTDE/x86 proxy. No installation or game access."""


def build(api, output, target):
    if target != "x86-windows-gnu":
        raise ValueError("PTDE's native hooks and proxy require x86")
    objects = []
    for name in ("buffer", "hook", "trampoline", "hde/hde32"):
        source = api.ROOT / "work/minhook/src" / (name + ".c")
        obj = output.parent / (source.stem + ".o")
        api.run([api.ZIG, "cc", "-target", "x86-windows-gnu", "-O2", "-c", source, "-o", obj])
        objects.append(obj)
    host = api.FRAMEWORK / "hosts/ds1ptde"
    api.compile_cpp(
        host / "runtime.cpp",
        output,
        [output.parent],
        extra=[
            "-shared",
            host / "proxy.def",
            *objects,
            "-ld3d9",
            "-ldxguid",
            "-luser32",
            "-lwinmm",
            "-lgdi32",
            "-lshell32",
        ],
        target="x86-windows-gnu",
    )
