"""Compatibility entry point for the character-owned runtime builder."""
from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[1]
subprocess.run([sys.executable,root/'framework/characters/mario64/build_runtime.py','--output',root/'work/build'],check=True)
