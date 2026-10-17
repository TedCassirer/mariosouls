"""Build a source-based Windows player installer locally, without GitHub Actions.

Only our tracked source and an official Python runtime are embedded. Guest
geometry/runtime, toolchains and private game exports are not release payloads.
"""

from pathlib import Path
import argparse
import hashlib
import io
import json
import os
import subprocess
import sys
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
LOCK = ROOT / "release/installer.lock.json"
CACHE = ROOT / "work/installer-build/downloads"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def fetch(url):
    with urllib.request.urlopen(url, timeout=60) as response:
        return response.read()


def json_url(url):
    return json.loads(fetch(url))


def prepare_lock():
    """Explicit maintainer operation: resolve once, then review/commit the pins."""
    CACHE.mkdir(parents=True, exist_ok=True)
    python_name = "python-3.13.16-embed-amd64.zip"
    python_url = "https://www.python.org/ftp/python/3.13.16/" + python_name
    python_archive = CACHE / python_name
    if not python_archive.exists():
        python_archive.write_bytes(fetch(python_url))
    subprocess.run(
        [
            sys.executable,
            "-m",
            "pip",
            "download",
            "--disable-pip-version-check",
            "--dest",
            str(CACHE),
            "--only-binary=:all:",
            "--platform",
            "win_amd64",
            "--python-version",
            "3.13",
            "--implementation",
            "cp",
            "--abi",
            "cp313",
            "pip==26.2.1",
            "numpy==2.2.6",
            "Pillow==12.3.0",
            "soulstruct-havok==1.2.2",
        ],
        check=True,
    )
    wheels = []
    for path in sorted(CACHE.glob("*.whl")):
        with zipfile.ZipFile(path) as bundle:
            metadata_name = next(n for n in bundle.namelist() if n.endswith(".dist-info/METADATA"))
            fields = bundle.read(metadata_name).decode().splitlines()
            name = next(line[6:] for line in fields if line.startswith("Name: "))
            version = next(line[9:] for line in fields if line.startswith("Version: "))
        info = json_url("https://pypi.org/pypi/" + name + "/" + version + "/json")
        asset = next(a for a in info["urls"] if a["filename"] == path.name)
        data = path.read_bytes()
        if sha(data) != asset["digests"]["sha256"]:
            raise ValueError("PyPI wheel hash mismatch")
        wheels.append(
            dict(
                name=name,
                version=version,
                filename=path.name,
                url=asset["url"],
                sha256=sha(data),
                size=len(data),
            )
        )
    git = json_url(
        "https://api.github.com/repos/git-for-windows/git/releases/tags/v2.56.0.windows.2"
    )
    git = next(a for a in git["assets"] if a["name"] == "MinGit-2.56.0.2-64-bit.zip")
    lock = dict(
        schema=1,
        python=dict(
            filename=python_name,
            url=python_url,
            sha256=sha(python_archive.read_bytes()),
            size=python_archive.stat().st_size,
        ),
        git=dict(
            filename=git["name"],
            url=git["browser_download_url"],
            sha256=git["digest"].removeprefix("sha256:"),
            size=git["size"],
        ),
        soulstruct=dict(
            path="work/soulstruct-source",
            url="https://github.com/Grimrukh/soulstruct.git",
            commit="12b69189a2ccebbc623a1b6565be89a18d6c9958",
        ),
        wheels=wheels,
    )
    LOCK.write_text(json.dumps(lock, indent=2) + "\n", encoding="utf-8")
    print("Prepared installer download lock; review it before building a release.")


def build():
    if os.name != "nt":
        raise RuntimeError("Build the .exe on Windows with the .NET Framework C# compiler")
    status = subprocess.check_output(["git", "status", "--porcelain"], cwd=ROOT, text=True)
    if status.strip():
        raise ValueError("Commit the reviewed source and lock file before building an installer")
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    version = json.loads((ROOT / "release/project.json").read_text())["version"]
    out = ROOT / "outputs/release" / version
    out.mkdir(parents=True, exist_ok=True)
    stage = ROOT / "work/installer-build" / commit
    stage.mkdir(parents=True, exist_ok=True)
    lock = json.loads(LOCK.read_text())
    python_archive = CACHE / lock["python"]["filename"]
    if not python_archive.exists():
        python_archive.parent.mkdir(parents=True, exist_ok=True)
        python_archive.write_bytes(fetch(lock["python"]["url"]))
    if sha(python_archive.read_bytes()) != lock["python"]["sha256"]:
        raise ValueError("Embedded Python hash mismatch")
    tracked = subprocess.check_output(["git", "archive", "--format=zip", "HEAD"], cwd=ROOT)
    files = {}
    source = io.BytesIO()
    with (
        zipfile.ZipFile(io.BytesIO(tracked)) as src,
        zipfile.ZipFile(source, "w", zipfile.ZIP_DEFLATED) as dst,
    ):
        for entry in src.infolist():
            if entry.is_dir():
                continue
            path = entry.filename
            if path.startswith(("work/", "outputs/")) or Path(path).suffix.lower() in (
                ".dll",
                ".exe",
                ".z64",
                ".scprop",
                ".bin",
            ):
                raise ValueError("Private/binary data must not enter the installer: " + path)
            data = src.read(entry)
            files[path] = sha(data)
            dst.writestr(path, data)
        dst.writestr("release/source.json", json.dumps(dict(commit=commit, files=files), indent=2))
    source_zip = stage / "source.zip"
    source_zip.write_bytes(source.getvalue())
    manifest = dict(
        schema=1,
        version=version,
        commit=commit,
        target="windows-x64-installer/ptde-x86",
        source_sha256=sha(source.getvalue()),
        python_sha256=lock["python"]["sha256"],
        distribution="source-build; no game/ROM assets or prebuilt guest runtime",
        signed=False,
    )
    manifest_file = stage / "manifest.json"
    manifest_file.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    exe = out / ("MarioSouls-" + version + "-Setup.exe")
    compiler = Path(os.environ["WINDIR"]) / "Microsoft.NET/Framework64/v4.0.30319/csc.exe"
    subprocess.run(
        [
            str(compiler),
            "/nologo",
            "/target:winexe",
            "/platform:x64",
            "/optimize+",
            "/out:" + str(exe),
            "/reference:System.Windows.Forms.dll",
            "/reference:System.Drawing.dll",
            "/reference:System.IO.Compression.dll",
            "/reference:System.IO.Compression.FileSystem.dll",
            "/reference:System.Web.Extensions.dll",
            "/resource:" + str(source_zip) + ",source.zip",
            "/resource:" + str(python_archive) + ",python.zip",
            "/resource:" + str(manifest_file) + ",manifest.json",
            str(ROOT / "framework/hosts/ds1ptde/installer.cs"),
        ],
        check=True,
    )
    source_out = out / ("MarioSouls-" + version + "-source.zip")
    source_out.write_bytes(source.getvalue())
    manifest["exe_sha256"] = sha(exe.read_bytes())
    manifest["exe_size"] = exe.stat().st_size
    (out / "installer-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    (out / "SHA256SUMS.txt").write_text(
        "\n".join(
            sha(path.read_bytes()) + "  " + path.name
            for path in [exe, source_out, out / "installer-manifest.json"]
        )
        + "\n",
        encoding="utf-8",
    )
    print("Built " + str(exe))
    print("No release was uploaded or published; inspect and verify this candidate first.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepare-lock", action="store_true")
    args = parser.parse_args()
    prepare_lock() if args.prepare_lock else build()
