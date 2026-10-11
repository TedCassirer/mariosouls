"""Compatibility entry point. New host source lives in framework/hosts/ds1ptde."""
from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[1]
subprocess.run([sys.executable,root/'tools/framework.py','build'],check=True)
print('Output: work/framework-build. Install with tools/framework.py after tests.')
