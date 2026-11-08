#!/usr/bin/env python3
"""Offline ORBYX batch runner with content-verified resume. Python 3.10+."""
from __future__ import annotations
import argparse
import hashlib
import fcntl
import json
import os
import stat
from pathlib import Path
import subprocess
import sys
import tempfile


ITALIAN = {
    "Symlink not permitted: ": "Collegamento simbolico non consentito: ",
    "Missing path: ": "Percorso mancante: ",
    "Non-regular file: ": "File non normale: ",
    "Manifest must be a regular non-symlink file": "Il manifest deve essere un file normale, non un collegamento simbolico",
    "Manifest exceeds 1 MiB": "Il manifest supera 1 MiB",
    "Manifest must contain only a jobs list": "Il manifest deve contenere soltanto una lista jobs",
    "Expected 1–10000 jobs": "Sono richiesti da 1 a 10000 lavori",
    "Invalid job fields": "Campi del lavoro non validi",
    "Invalid string field: ": "Campo di testo non valido: ",
    "Duplicate or excessive job id": "Identificativo del lavoro duplicato o troppo lungo",
    "Mode must be generate or calibrate": "La modalità deve essere generate o calibrate",
    "cpu must be boolean": "cpu deve essere un valore booleano",
    "Conflicting output directories": "Cartelle di output in conflitto",
    "Output conflicts with an input or profile": "L'output è in conflitto con un input o un profilo",
    "State and lock must be regular non-symlink files": "Stato e lock devono essere file normali, non collegamenti simbolici",
    "State and lock must not have hardlink aliases": "Stato e lock non devono avere alias tramite collegamenti fisici",
    "State or lock conflicts with a manifest, engine or job path": "Stato o lock in conflitto con manifest, motore o percorsi dei lavori",
    "Output conflicts with the manifest or engine": "L'output è in conflitto con il manifest o il motore",
    "State already exists; use --resume or choose a new state path": "Lo stato esiste già; usare --resume oppure scegliere un nuovo percorso di stato",
    "State exceeds 16 MiB": "Lo stato supera 16 MiB",
    "Invalid state schema": "Schema dello stato non valido",
    "Output exists but cannot be verified; choose a new output directory": "L'output esiste ma non è verificabile; scegliere una nuova cartella di output",
    "Engine exited with status ": "Il motore è terminato con codice ",
    "Engine did not produce an output directory": "Il motore non ha prodotto una cartella di output",
    "Input, profile or engine changed during processing": "Input, profilo o motore modificati durante l'elaborazione",
    "Lock must be a regular file": "Il lock deve essere un file normale",
    "Another batch runner owns this state": "Un altro processo batch sta utilizzando questo stato",
    "skipped (verified)": "saltato (verificato)", "completed": "completato", "failed: ": "errore: ",
    "the following arguments are required: ": "sono richiesti i seguenti argomenti: ",
    "unrecognized arguments: ": "argomenti non riconosciuti: ",
    "argument --lang: invalid choice: ": "argomento --lang: scelta non valida: ",
    "argument --lang: expected one argument": "argomento --lang: è richiesto un valore",
}


def translate(message: str, language: str) -> str:
    if language != "it":
        return message
    for source, translated in ITALIAN.items():
        if message.startswith(source):
            return translated + message[len(source):]
    # Preserve external OS/JSON diagnostic details for reproducible support reports.
    return "Dettaglio tecnico: " + message


class LocalizedParser(argparse.ArgumentParser):
    language = "en"

    def format_help(self):
        value = super().format_help()
        return value.replace("usage:", "uso:", 1) if self.language == "it" else value

    def format_usage(self):
        value = super().format_usage()
        return value.replace("usage:", "uso:", 1) if self.language == "it" else value

    def error(self, message):
        self.print_usage(sys.stderr)
        label = "errore" if self.language == "it" else "error"
        self.exit(2, f"{self.prog}: {label}: {translate(message, self.language)}\n")


def digest(path: Path) -> str:
    """Digest names and bytes; reject symlinks to keep the acquisition bounded."""
    if path.is_symlink():
        raise ValueError(f"Symlink not permitted: {path}")
    if path.is_file():
        h = hashlib.sha256()
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                h.update(block)
        return h.hexdigest()
    if not path.is_dir():
        raise ValueError(f"Missing path: {path}")
    h = hashlib.sha256()
    for item in sorted(path.rglob("*")):
        if item.is_symlink():
            raise ValueError(f"Symlink not permitted: {item}")
        name = item.relative_to(path).as_posix().encode("utf-8")
        h.update(len(name).to_bytes(8, "big")); h.update(name)
        if item.is_file():
            h.update(b"F"); h.update(bytes.fromhex(digest(item)))
        elif item.is_dir():
            h.update(b"D")
        else:
            raise ValueError(f"Non-regular file: {item}")
    return h.hexdigest()


def atomic_json(path: Path, value: dict) -> None:
    fd, name = tempfile.mkstemp(prefix=".orbyx-state-", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            json.dump(value, stream, ensure_ascii=False, indent=2)
            stream.write("\n"); stream.flush(); os.fsync(stream.fileno())
        os.replace(name, path)
        directory = os.open(path.parent, os.O_RDONLY)
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def resolve(base: Path, value: str) -> Path:
    path = Path(value).expanduser()
    return (base / path if not path.is_absolute() else path).absolute()


def load_manifest(path: Path) -> list[dict]:
    if path.is_symlink() or not path.is_file():
        raise ValueError("Manifest must be a regular non-symlink file")
    if path.stat().st_size > 1024 * 1024:
        raise ValueError("Manifest exceeds 1 MiB")
    document = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(document, dict) or set(document) != {"jobs"}:
        raise ValueError("Manifest must contain only a jobs list")
    jobs = document["jobs"]
    if not isinstance(jobs, list) or not jobs or len(jobs) > 10000:
        raise ValueError("Expected 1–10000 jobs")
    identifiers: set[str] = set()
    outputs: list[Path] = []
    normalized = []
    allowed = {"id", "satellite", "input", "output", "config", "mode", "cpu"}
    required = {"id", "satellite", "input", "output", "config"}
    for job in jobs:
        if not isinstance(job, dict) or not required <= set(job) or set(job) - allowed:
            raise ValueError("Invalid job fields")
        for key in required:
            if not isinstance(job[key], str) or not job[key] or "\0" in job[key]:
                raise ValueError(f"Invalid string field: {key}")
        if len(job["id"]) > 128 or job["id"] in identifiers:
            raise ValueError("Duplicate or excessive job id")
        identifiers.add(job["id"])
        mode = job.get("mode", "generate")
        if mode not in ("generate", "calibrate") or not isinstance(mode, str):
            raise ValueError("Mode must be generate or calibrate")
        if type(job.get("cpu", False)) is not bool:
            raise ValueError("cpu must be boolean")
        item = dict(job, mode=mode, cpu=job.get("cpu", False))
        for key in ("input", "output", "config"):
            item[key] = str(resolve(path.parent, job[key]))
        output = Path(item["output"]).resolve()
        for other in outputs:
            if output == other or output in other.parents or other in output.parents:
                raise ValueError("Conflicting output directories")
        outputs.append(output)
        normalized.append(item)
    # Outputs cannot mutate another job's inputs or calibration profile.
    for output in outputs:
        for job in normalized:
            for key in ("input", "config"):
                protected = Path(job[key]).resolve()
                if output == protected or output in protected.parents or protected in output.parents:
                    raise ValueError("Output conflicts with an input or profile")
    return normalized


def validate_control_paths(jobs: list[dict], manifest: Path, state: Path,
                           executable: Path) -> None:
    """Validate every writable control path before even creating the lock file."""
    controls = (state, state.with_name(state.name + ".lock"))
    protected = [manifest, executable]
    protected.extend(Path(job[key]) for job in jobs for key in ("input", "output", "config"))
    for control in controls:
        if control.is_symlink() or (control.exists() and not control.is_file()):
            raise ValueError("State and lock must be regular non-symlink files")
        if control.exists() and control.stat().st_nlink != 1:
            raise ValueError("State and lock must not have hardlink aliases")
        resolved = control.resolve()
        for target in protected:
            canonical = target.resolve()
            if (resolved == canonical or canonical in resolved.parents
                    or resolved in canonical.parents
                    or (control.exists() and target.exists() and control.samefile(target))):
                raise ValueError("State or lock conflicts with a manifest, engine or job path")
    for job in jobs:
        output = Path(job["output"]).resolve()
        for target in (manifest.resolve(), executable.resolve()):
            if output == target or output in target.parents or target in output.parents:
                raise ValueError("Output conflicts with the manifest or engine")


def fingerprint(job: dict, executable: Path) -> str:
    value = {"job": job, "input": digest(Path(job["input"])),
             "config": digest(Path(job["config"])), "engine": digest(executable)}
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def run(manifest: Path, state_path: Path, executable: Path, resume: bool, language: str = "en") -> int:
    jobs = load_manifest(manifest)
    validate_control_paths(jobs, manifest, state_path, executable)
    if state_path.exists() and not resume:
        raise ValueError("State already exists; use --resume or choose a new state path")
    state = {"version": 1, "jobs": {}}
    if resume and state_path.exists():
        if state_path.stat().st_size > 16 * 1024 * 1024:
            raise ValueError("State exceeds 16 MiB")
        state = json.loads(state_path.read_text(encoding="utf-8"))
        if (not isinstance(state, dict) or state.get("version") != 1
                or not isinstance(state.get("jobs"), dict)):
            raise ValueError("Invalid state schema")
    errors = False
    for job in jobs:
        identifier = job["id"]
        output = Path(job["output"])
        process = None
        previous = state["jobs"].get(identifier, {})
        try:
            current = fingerprint(job, executable)
            if (resume and isinstance(previous, dict) and previous.get("status") == "completed"
                    and previous.get("fingerprint") == current and output.is_dir()
                    and previous.get("output_digest") == digest(output)):
                if "last_attempt" in previous:
                    previous.pop("last_attempt")
                    atomic_json(state_path, state)
                print(f"{identifier}: {translate('skipped (verified)', language)}", flush=True)
                continue
            # Existing products cannot be overwritten, even after input or output drift.
            if output.exists() or output.is_symlink():
                raise ValueError("Output exists but cannot be verified; choose a new output directory")
            entry = {"status": "running", "fingerprint": current}
            state["jobs"][identifier] = entry
            atomic_json(state_path, state)
            command = [str(executable), job["mode"], "--sat", job["satellite"],
                       "--directory", job["input"], "--output", job["output"],
                       "--config", job["config"], "--lang", language]
            if job["cpu"]:
                command.append("--cpu")
            process = subprocess.Popen(command, shell=False)
            code = process.wait()
            if code:
                raise RuntimeError(f"Engine exited with status {code}")
            if not output.is_dir() or output.is_symlink():
                raise RuntimeError("Engine did not produce an output directory")
            if fingerprint(job, executable) != current:
                raise RuntimeError("Input, profile or engine changed during processing")
            entry.update(status="completed", output_digest=digest(output))
            print(f"{identifier}: {translate('completed', language)}", flush=True)
        except KeyboardInterrupt:
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill(); process.wait()
            interrupted = {"status": "interrupted"}
            state["jobs"][identifier] = (dict(previous, last_attempt=interrupted)
                if isinstance(previous, dict) and previous.get("status") == "completed"
                else interrupted)
            atomic_json(state_path, state)
            return 130
        except (ValueError, RuntimeError, OSError) as error:
            errors = True
            failure = {"status": "failed", "error": str(error)}
            if isinstance(previous, dict) and previous.get("status") == "completed":
                # Keep the last verified completion so fixing temporary drift can
                # resume safely; a failed verification must not erase provenance.
                state["jobs"][identifier] = dict(previous, last_attempt=failure)
            else:
                state["jobs"][identifier] = failure
            print(f"{identifier}: {translate('failed: ', language)}{translate(str(error), language)}", file=sys.stderr, flush=True)
        atomic_json(state_path, state)
    return 1 if errors else 0


def main() -> int:
    language = "en"
    for index, value in enumerate(sys.argv[1:], 1):
        if value == "--lang" and index + 1 < len(sys.argv):
            language = sys.argv[index + 1]
        elif value.startswith("--lang="):
            language = value.split("=", 1)[1]
    italian = language == "it"
    description = ("Esecutore batch offline ORBYX con ripresa verificata tramite digest. Python 3.10+."
                   if italian else __doc__)
    parser = LocalizedParser(description=description, add_help=False)
    parser.language = language
    parser._positionals.title = "argomenti posizionali" if italian else "positional arguments"
    parser._optionals.title = "opzioni" if italian else "options"
    parser.add_argument("-h", "--help", action="help", help="mostra questa guida ed esce" if italian else "show this help message and exit")
    parser.add_argument("manifest", type=Path, help="manifest JSON dei lavori" if italian else "JSON jobs manifest")
    parser.add_argument("--state", type=Path, help="file JSON di stato" if italian else "state JSON file")
    parser.add_argument("--orbyx", required=True, type=Path, help="percorso eseguibile ORBYX" if italian else "ORBYX executable path")
    parser.add_argument("--resume", action="store_true", help="riprende i lavori verificando i digest" if italian else "resume jobs after verifying digests")
    parser.add_argument("--lang", choices=("en", "it"), default="en", help="lingua dei messaggi" if italian else "message language")
    args = parser.parse_args()
    manifest = args.manifest.absolute()
    state = args.state.absolute() if args.state else manifest.with_suffix(".state.json")
    try:
        # Advisory lock prevents two runners from racing the same durable state.
        # Keep the lock inode after release; unlinking permits lock-inode races.
        validate_control_paths(load_manifest(manifest), manifest, state, args.orbyx.absolute())
        lock_path = state.with_name(state.name + ".lock")
        descriptor = os.open(lock_path, os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW | os.O_NONBLOCK, 0o600)
        if not stat.S_ISREG(os.fstat(descriptor).st_mode) or os.fstat(descriptor).st_nlink != 1:
            os.close(descriptor)
            raise ValueError("Lock must be a regular file")
        with os.fdopen(descriptor, "w") as lock:
            try:
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BlockingIOError as error:
                raise ValueError("Another batch runner owns this state") from error
            return run(manifest, state, args.orbyx.absolute(), args.resume, args.lang)
    except (ValueError, OSError) as error:
        parser.error(str(error))
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
