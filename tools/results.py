"""Persist named offline checks and failure logs for local runs and CI."""

import json
from pathlib import Path
import subprocess
import time
import xml.etree.ElementTree as ET


class Results:
    def __init__(self, directory):
        self.directory = Path(directory)
        self.directory.mkdir(parents=True, exist_ok=True)
        self.checks = []

    def run(self, args, **kwargs):
        args = [str(a) for a in args]
        # Names omit absolute checkout paths; logs remain private CI artifacts.
        name = Path(
            args[1]
            if len(args) > 1 and Path(args[0]).stem.lower().startswith("python")
            else args[0]
        ).name
        if name == "tests" and len(args) > 2:
            name = args[2]
        log = self.directory / (str(len(self.checks) + 1).zfill(2) + "-" + name + ".log")
        started = time.monotonic()
        status = "passed"
        try:
            with log.open("w", encoding="utf-8") as out:
                subprocess.run(args, check=True, stdout=out, stderr=subprocess.STDOUT, **kwargs)
        except BaseException:
            status = "failed"
            print(log.read_text(encoding="utf-8", errors="replace")[-16000:])
            raise
        finally:
            self.checks.append(
                dict(
                    name=name,
                    status=status,
                    seconds=round(time.monotonic() - started, 3),
                    log=log.name,
                )
            )
            self.write(status="running" if status == "passed" else "failed")
            print(status.upper() + ": " + name)

    def write(self, status, error=None):
        data = dict(
            schema=1,
            status=status,
            scope="asset-free-shared",
            authorizes_install=False,
            checks=self.checks,
        )
        if error:
            data["error"] = str(error)
        (self.directory / "results.json").write_text(json.dumps(data, indent=2), encoding="utf-8")
        failed = sum(c["status"] == "failed" for c in self.checks)
        infrastructure = bool(error and not failed)
        suite = ET.Element(
            "testsuite",
            name="framework-shared",
            tests=str(len(self.checks) + int(infrastructure)),
            failures=str(failed + int(infrastructure)),
            time=str(round(sum(c["seconds"] for c in self.checks), 3)),
        )
        for check in self.checks:
            case = ET.SubElement(
                suite,
                "testcase",
                name=check["name"],
                classname="framework.shared",
                time=str(check["seconds"]),
            )
            ET.SubElement(case, "system-out").text = "Log: " + check["log"]
            if check["status"] == "failed":
                ET.SubElement(case, "failure", message="Check failed").text = "See " + check["log"]
        if infrastructure:
            ET.SubElement(
                ET.SubElement(suite, "testcase", name="suite-infrastructure"),
                "failure",
                message="Suite did not complete",
            ).text = str(error)
        ET.ElementTree(suite).write(
            self.directory / "junit.xml", encoding="utf-8", xml_declaration=True
        )
