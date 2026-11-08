"""Verify the desktop queue delegates to the verified batch runner.

The GUI binary's --queue mode runs the durable, resumable batch runner and
exits with its status. This test drives it headlessly, checks that each queued
job produces the same product bytes as a direct CLI run, and that a --resume
pass over unchanged inputs succeeds without losing data.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

fixture_tool, gui, cli, runner = (str(Path(a)) for a in sys.argv[1:5])
environment = dict(os.environ, QT_QPA_PLATFORM="offscreen")

with tempfile.TemporaryDirectory(prefix="orbyx-gui-queue-") as directory:
    root = Path(directory)
    environment["XDG_CONFIG_HOME"] = str(root / "settings")
    fixture = root / "fixture"
    subprocess.run([fixture_tool, "--fixture", str(fixture)], check=True, env=environment, timeout=20)
    inp, profile = fixture / "input", fixture / "profile.json"
    manifest, state = root / "queue.json", root / "queue.state.json"

    jobs = [{"id": name, "satellite": "L2", "input": str(inp), "config": str(profile),
             "output": str(root / ("out-" + name)), "cpu": True} for name in ("one", "two")]
    manifest.write_text(json.dumps({"jobs": jobs}))

    queue = [gui, "--queue", str(manifest), "--state", str(state),
             "--batch-runner", runner, "--orbyx", cli, "--lang", "en"]
    assert subprocess.run(queue, env=environment, timeout=120).returncode == 0

    subprocess.run([cli, "generate", "--sat", "L2", "--directory", str(inp), "--config", str(profile),
                    "--output", str(root / "cli-out"), "--cpu"], check=True, env=environment, timeout=60)
    expected = sorted(p.relative_to(root / "cli-out") for p in (root / "cli-out").rglob("*") if p.is_file())
    assert len(expected) == 8, expected
    for name in ("one", "two"):
        out = root / ("out-" + name)
        actual = sorted(p.relative_to(out) for p in out.rglob("*") if p.is_file())
        assert actual == expected, (name, actual, expected)
        for path in actual:
            assert (out / path).read_bytes() == (root / "cli-out" / path).read_bytes(), (name, path)

    # Resume over unchanged inputs: digests verify, nothing is lost or overwritten.
    assert subprocess.run(queue + ["--resume"], env=environment, timeout=120).returncode == 0
    recorded = json.loads(state.read_text())
    assert recorded["jobs"]["one"]["status"] == "completed", recorded
    assert recorded["jobs"]["two"]["status"] == "completed", recorded

print("GUI queue delegated to batch runner; products match CLI and resume verified")
