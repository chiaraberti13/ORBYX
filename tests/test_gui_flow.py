"""Verify the desktop worker generates the same product bytes as the headless CLI."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

fixture_tool, gui, cli = map(str, map(Path, sys.argv[1:]))
environment = dict(os.environ, QT_QPA_PLATFORM="offscreen")
with tempfile.TemporaryDirectory(prefix="orbyx-gui-flow-") as directory:
    root = Path(directory) / "fixture"
    environment["XDG_CONFIG_HOME"] = str(Path(directory)/"settings")
    subprocess.run([fixture_tool, "--fixture", str(root)], check=True, env=environment, timeout=20)
    flags = ["--satellite", "L2", "--input", str(root/"input"), "--config", str(root/"profile.json"), "--output", str(root/"gui-output"), "--cpu", "--run-and-exit"]
    subprocess.run([gui, *flags], check=True, env=environment, timeout=40)
    subprocess.run([cli, "generate", "--sat", "L2", "--directory", str(root/"input"), "--config", str(root/"profile.json"), "--output", str(root/"cli-output"), "--cpu"], check=True, env=environment, timeout=40)
    expected = sorted(p.relative_to(root/"cli-output") for p in (root/"cli-output").rglob("*") if p.is_file())
    actual = sorted(p.relative_to(root/"gui-output") for p in (root/"gui-output").rglob("*") if p.is_file())
    assert actual == expected and len(actual) == 8, (actual, expected)
    for path in actual:
        assert (root/"gui-output"/path).read_bytes() == (root/"cli-output"/path).read_bytes(), path
    # Existing destinations fail through the GUI worker, without a blocking modal.
    result = subprocess.run([gui, *flags], env=environment, timeout=40)
    assert result.returncode == 2, result.returncode
print("GUI worker and CLI products match; overwrite refused")
