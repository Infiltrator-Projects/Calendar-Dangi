#!/usr/bin/env python3
"""The native installer owns its illustrated PDF generator dependencies."""
import importlib.util
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("release", Path(__file__).resolve().parents[1] / "packaging/release.py")
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)
for code, expected in ((1, True), (0, False)):
    with (
        patch.object(worker.shutil, "which", return_value="/usr/bin/tool"),
        patch.object(worker.subprocess, "check_output", return_value="en_US.utf8\n"),
        patch.object(worker.subprocess, "run", return_value=SimpleNamespace(returncode=code)),
    ):
        assert ("python3-reportlab" in worker.check_requirements()) is expected
print("PDF dependency detection passed")
