"""Failure-path coverage for the CI result recorder."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from results import Results


class Reports(unittest.TestCase):
    def test_success_and_failure_are_persisted(self):
        with tempfile.TemporaryDirectory() as tmp:
            r = Results(tmp)
            r.run([sys.executable, "-c", 'print("fixture passed")'])
            with self.assertRaises(subprocess.CalledProcessError):
                r.run([sys.executable, "-c", 'print("fixture failed"); raise SystemExit(3)'])
            r.write("failed")
            data = json.loads((Path(tmp) / "results.json").read_text())
            self.assertEqual([c["status"] for c in data["checks"]], ["passed", "failed"])
            self.assertFalse(data["authorizes_install"])
            xml = ET.parse(Path(tmp) / "junit.xml").getroot()
            self.assertEqual(xml.get("failures"), "1")
            self.assertIn("fixture failed", (Path(tmp) / data["checks"][1]["log"]).read_text())

    def test_infrastructure_failure_has_junit_entry(self):
        with tempfile.TemporaryDirectory() as tmp:
            r = Results(tmp)
            r.write("failed", RuntimeError("compile <failed> & details"))
            xml = ET.parse(Path(tmp) / "junit.xml").getroot()
            self.assertEqual(xml.get("tests"), "1")
            self.assertEqual(xml.get("failures"), "1")
            self.assertEqual(xml.find("testcase/failure").text, "compile <failed> & details")


if __name__ == "__main__":
    unittest.main()
