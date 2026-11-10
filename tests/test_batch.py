"""Batch contract tests using an external deterministic engine executable."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

RUNNER = Path(__file__).resolve().parents[1] / "scripts/orbyx_batch.py"


class BatchTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.engine = self.root / "engine"
        self.engine.write_text("#!" + sys.executable + "\n" + '''import pathlib,sys
args=sys.argv[1:]
satellite=args[args.index('--sat')+1]
if satellite=='FAIL': sys.exit(9)
p=pathlib.Path(args[args.index('--output')+1]); p.mkdir()
(p/'result').write_text('synthetic-success')
(p/'args').write_text('\\n'.join(args))
with (pathlib.Path(__file__).parent/'calls').open('a') as s:s.write(satellite+'\\n')
''')
        self.engine.chmod(0o700)
        (self.root / "input").mkdir()
        (self.root / "input/channel.png").write_bytes(b"synthetic-source")
        (self.root / "profile.json").write_text("{}")
        self.manifest = self.root / "batch.json"

    def tearDown(self):
        self.temp.cleanup()

    def job(self, name, satellite="L2"):
        return {"id": name, "satellite": satellite, "input": "input",
                "output": name + "-output", "config": "profile.json", "cpu": True}

    def invoke(self, jobs, resume=False, language="en"):
        self.manifest.write_text(json.dumps({"jobs": jobs}))
        command = [sys.executable, str(RUNNER), str(self.manifest), "--orbyx", str(self.engine), "--lang", language]
        if resume:
            command.append("--resume")
        return subprocess.run(command, cwd="/", capture_output=True, text=True)

    def test_italian_help_and_job_diagnostics(self):
        help_result = subprocess.run([sys.executable, str(RUNNER), "--lang", "it", "--help"], capture_output=True, text=True)
        self.assertEqual(help_result.returncode, 0)
        self.assertIn("Esecutore batch offline", help_result.stdout)
        self.assertIn("riprende i lavori", help_result.stdout)
        jobs = [self.job("one")]
        result = self.invoke(jobs, language="it")
        self.assertEqual(result.returncode, 0)
        self.assertIn("one: completato", result.stdout)
        result = self.invoke(jobs, True, language="it")
        self.assertEqual(result.returncode, 0)
        self.assertIn("saltato (verificato)", result.stdout)
        # Locale is presentation only: changing it must not invalidate a completion.
        self.assertEqual(self.invoke(jobs, True, language="en").returncode, 0)
        self.assertEqual((self.root / "calls").read_text(), "L2\n")

    def test_success_and_verified_resume(self):
        jobs = [self.job("one")]
        self.assertEqual(self.invoke(jobs).returncode, 0)
        self.assertEqual(self.invoke(jobs, True).returncode, 0)
        self.assertEqual((self.root / "calls").read_text(), "L2\n")
        state = json.loads((self.root / "batch.state.json").read_text())
        self.assertEqual(state["jobs"]["one"]["status"], "completed")

    def test_selected_products_are_validated_and_forwarded(self):
        job = self.job("selected")
        job["products"] = ["MSUGS_VIS1", "MSUGS_VIS-3_Aligned"]
        self.assertEqual(self.invoke([job]).returncode, 0)
        arguments = (self.root / "selected-output/args").read_text().splitlines()
        self.assertEqual([arguments[i + 1] for i, value in enumerate(arguments) if value == "--product"], job["products"])
        bad = self.job("bad-products"); bad["products"] = ["../MSUGS_VIS1"]
        self.assertNotEqual(self.invoke([bad]).returncode, 0)

    def test_failure_does_not_block_later_job(self):
        result = self.invoke([self.job("bad", "FAIL"), self.job("good")])
        self.assertEqual(result.returncode, 1)
        self.assertTrue((self.root / "good-output/result").is_file())
        state = json.loads((self.root / "batch.state.json").read_text())
        self.assertEqual(state["jobs"]["bad"]["status"], "failed")
        self.assertEqual(state["jobs"]["good"]["status"], "completed")

    def test_input_drift_cannot_skip_or_overwrite(self):
        jobs = [self.job("one")]
        self.assertEqual(self.invoke(jobs).returncode, 0)
        (self.root / "input/channel.png").write_bytes(b"changed-source")
        self.assertEqual(self.invoke(jobs, True).returncode, 1)
        self.assertEqual((self.root / "calls").read_text(), "L2\n")
        self.assertEqual((self.root / "one-output/result").read_text(), "synthetic-success")

    def test_output_drift_cannot_skip(self):
        jobs = [self.job("one")]
        self.assertEqual(self.invoke(jobs).returncode, 0)
        (self.root / "one-output/result").write_text("tampered")
        self.assertEqual(self.invoke(jobs, True).returncode, 1)
        self.assertEqual((self.root / "one-output/result").read_text(), "tampered")
        self.assertEqual((self.root / "calls").read_text(), "L2\n")

    def test_config_and_engine_drift_cannot_skip(self):
        for filename in ("profile.json", "engine"):
            with self.subTest(filename=filename):
                name = filename.replace(".", "_")
                jobs = [self.job(name)]
                self.assertEqual(self.invoke(jobs, filename == "engine").returncode, 0)
                file = self.root / filename
                file.write_text(file.read_text() + "\n")
                self.assertEqual(self.invoke(jobs, True).returncode, 1)

    def test_conflicting_outputs_and_bad_types(self):
        first, second = self.job("one"), self.job("two")
        second["output"] = "one-output/nested"
        self.assertEqual(self.invoke([first, second]).returncode, 2)
        self.assertFalse((self.root / "calls").exists())
        first["cpu"] = "false"
        self.assertEqual(self.invoke([first]).returncode, 2)
        first = self.job("one")
        self.assertEqual(self.invoke([first, first]).returncode, 2)

    def test_failed_verification_preserves_completion_for_recovered_input(self):
        jobs = [self.job("one")]
        self.assertEqual(self.invoke(jobs).returncode, 0)
        source = self.root / "input/channel.png"
        original = source.read_bytes()
        source.write_bytes(b"temporary-drift")
        self.assertEqual(self.invoke(jobs, True).returncode, 1)
        state = json.loads((self.root / "batch.state.json").read_text())
        self.assertEqual(state["jobs"]["one"]["status"], "completed")
        self.assertEqual(state["jobs"]["one"]["last_attempt"]["status"], "failed")
        source.write_bytes(original)
        self.assertEqual(self.invoke(jobs, True).returncode, 0)
        self.assertEqual((self.root / "calls").read_text(), "L2\n")

    def test_existing_state_requires_resume_without_erasing_it(self):
        jobs = [self.job("one")]
        self.assertEqual(self.invoke(jobs).returncode, 0)
        state = self.root / "batch.state.json"
        original = state.read_bytes()
        self.assertEqual(self.invoke(jobs).returncode, 2)
        self.assertEqual(state.read_bytes(), original)
        self.assertEqual((self.root / "calls").read_text(), "L2\n")

    def test_control_path_conflicts_rejected_before_lock_creation(self):
        self.manifest.write_text(json.dumps({"jobs": [self.job("one")]}))
        for state in (self.root / "input/run-state", self.root / "engine",
                      self.root / "batch.json", self.root / "one-output/state"):
            with self.subTest(state=state):
                result = subprocess.run([sys.executable, str(RUNNER), str(self.manifest),
                    "--orbyx", str(self.engine), "--state", str(state)],
                    capture_output=True, text=True)
                self.assertEqual(result.returncode, 2)
                self.assertFalse(state.with_name(state.name + ".lock").exists())
        self.assertFalse((self.root / "calls").exists())

    def test_lock_alias_to_input_rejected_without_touching_input(self):
        source = self.root / "input/channel.png"
        lock = self.root / "batch.state.json.lock"
        os.link(source, lock)
        self.assertEqual(self.invoke([self.job("one")]).returncode, 2)
        self.assertEqual(source.read_bytes(), b"synthetic-source")
        self.assertFalse((self.root / "calls").exists())

    def test_nonregular_state_and_lock_rejected_without_blocking(self):
        for filename in ("batch.state.json", "batch.state.json.lock"):
            with self.subTest(filename=filename):
                fifo = self.root / filename
                os.mkfifo(fifo)
                self.assertEqual(self.invoke([self.job("one")]).returncode, 2)
                fifo.unlink()
        self.assertFalse((self.root / "calls").exists())

    def test_existing_lock_excludes_another_runner(self):
        import fcntl
        lock = self.root / "batch.state.json.lock"
        with lock.open("w") as stream:
            fcntl.flock(stream, fcntl.LOCK_EX | fcntl.LOCK_NB)
            self.assertEqual(self.invoke([self.job("one")]).returncode, 2)
        self.assertFalse((self.root / "calls").exists())

    def test_ambiguous_and_nonfinite_manifest_rejected(self):
        job = json.dumps(self.job("one"))
        valid = '{"jobs": [' + job + ']}'
        for invalid in ('{"jobs": [], "jobs": [' + job + ']}',
                        valid.replace('"cpu": true', '"cpu": NaN'),
                        valid.replace('"cpu": true', '"cpu": Infinity'),
                        valid.replace('"cpu": true', '"cpu": -Infinity'),
                        valid.replace('"cpu": true', '"cpu": 1e999'),
                        valid.replace('"id": "one"', '"id": "other", "id": "one"')):
            with self.subTest(invalid=invalid):
                self.manifest.write_text(invalid)
                result = subprocess.run([sys.executable, str(RUNNER), str(self.manifest),
                    "--orbyx", str(self.engine)], capture_output=True, text=True, timeout=10)
                self.assertEqual(result.returncode, 2)
                self.assertFalse((self.root / "calls").exists())
                self.assertFalse((self.root / "batch.state.json.lock").exists())

    def test_invalid_resume_state_preserved(self):
        jobs = [self.job("one")]
        self.assertEqual(self.invoke(jobs).returncode, 0)
        state = self.root / "batch.state.json"
        original = state.read_bytes()
        for invalid in ('{"version":1,"version":1,"jobs":{}}',
                        '{"version":true,"jobs":{}}',
                        '{"version":1,"jobs":{"one":{"status":NaN}}}',
                        '{"version":1,"jobs":{}}' + ' ' * (16 * 1024 * 1024),
                        'not JSON',
                        '[' * 2000 + ']' * 2000):
            with self.subTest(invalid=invalid[:60]):
                state.write_text(invalid)
                before = state.read_bytes()
                self.assertEqual(self.invoke(jobs, True).returncode, 2)
                self.assertEqual(state.read_bytes(), before)
                self.assertEqual((self.root / "calls").read_text(), "L2\n")
        state.write_bytes(original)
        self.assertEqual(self.invoke(jobs, True).returncode, 0)

    def test_manifest_size_and_encoding_rejected(self):
        for raw in (b" " * (1024 * 1024 + 1), b"\xff\xfe"):
            with self.subTest(size=len(raw)):
                self.manifest.write_bytes(raw)
                result = subprocess.run([sys.executable, str(RUNNER), str(self.manifest),
                    "--orbyx", str(self.engine)], capture_output=True, text=True, timeout=10)
                self.assertEqual(result.returncode, 2)
                self.assertFalse((self.root / "calls").exists())

    def test_input_symlink_rejected(self):
        (self.root / "input/link").symlink_to(self.root / "profile.json")
        self.assertEqual(self.invoke([self.job("one")]).returncode, 1)
        self.assertFalse((self.root / "one-output").exists())


if __name__ == "__main__":
    unittest.main()
