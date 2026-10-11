"""Run developer tests with the same import context as the public framework CLI."""

from pathlib import Path
import sys
import unittest

TESTS = Path(__file__).resolve().parent
sys.path[:0] = [str(TESTS), str(TESTS.parent)]

if __name__ == "__main__":
    loader = unittest.TestLoader()
    suite = (
        loader.loadTestsFromName(sys.argv[1]) if len(sys.argv) == 2 else loader.discover(str(TESTS))
    )
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    raise SystemExit(not result.wasSuccessful())
