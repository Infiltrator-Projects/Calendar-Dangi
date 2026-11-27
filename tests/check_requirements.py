#!/usr/bin/env python3
"""Check that missing shared PDF tools fail dependency admission."""
import importlib.util
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("provider_release", root / "packaging/release.py")
provider = importlib.util.module_from_spec(spec)
spec.loader.exec_module(provider)
for absent in ("jq", "ps2pdf", "iconv", "fold"):
    with patch.object(provider.shutil, "which", side_effect=lambda name: None if name == absent else "/usr/bin/tool"), patch.object(provider.subprocess, "check_output", return_value="en_US.utf8\n"), patch.object(provider.subprocess, "run", return_value=SimpleNamespace(returncode=0)):
        assert absent in provider.check_requirements(), absent
with patch.object(provider.shutil, "which", return_value="/usr/bin/tool"), patch.object(provider.subprocess, "check_output", return_value="en_US.utf8\n"), patch.object(provider.subprocess, "run", return_value=SimpleNamespace(returncode=0)):
    assert not provider.check_requirements()
print("Shared PDF dependency detection passed")
