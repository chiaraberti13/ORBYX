#!/usr/bin/env python3
"""CPU performance benchmark for the ORBYX engine. Python 3.10+, standard library.

Generates a synthetic MSU-GS dataset (reusing the simulation generator) and
times the real `calibrate` and `generate` CLI runs end-to-end on the CPU
backend, reporting the minimum and median wall-clock over several iterations.

The numbers are real CPU measurements on the host that runs this script; they
are NOT a GPU/OpenCL comparison (roadmap M3-03 GPU equivalence needs real GPU
hardware) and not a real-acquisition benchmark.
"""
from __future__ import annotations

import argparse
import importlib.util
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from pathlib import Path

_SIM = Path(__file__).resolve().parent / "orbyx_simulate.py"
_spec = importlib.util.spec_from_file_location("orbyx_simulate", _SIM)
sim = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(sim)


def time_run(orbyx: Path, args: list[str]) -> float:
    start = time.perf_counter()
    subprocess.run([str(orbyx), *args], check=True, capture_output=True, timeout=1200)
    return time.perf_counter() - start


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--orbyx", required=True, type=Path)
    parser.add_argument("--width", type=int, default=384)
    parser.add_argument("--active", type=int, default=384, help="content band height")
    parser.add_argument("--iterations", type=int, default=3)
    parser.add_argument("--satellite", default="BENCH")
    args = parser.parse_args()

    height = sim.CH2_OFFSET + args.active + 32
    work = Path(tempfile.mkdtemp(prefix="orbyx-bench-"))
    try:
        data = work / "acquisition"
        sim.build_dataset(data, args.width, height, args.active, shift_px=2)
        (work / "empty.json").write_text("{}")

        calibrate_times, generate_times = [], []
        profile = None
        for i in range(args.iterations):
            calib = work / f"calib-{i}"
            calibrate_times.append(time_run(args.orbyx, [
                "calibrate", "-s", args.satellite, "-d", str(data),
                "--config", str(work / "empty.json"), "--output", str(calib), "--cpu"]))
            if profile is None:
                profile = calib / "calibration.json"
            gen = work / f"gen-{i}"
            generate_times.append(time_run(args.orbyx, [
                "generate", "-s", args.satellite, "-d", str(data),
                "--config", str(profile), "--output", str(gen), "--cpu"]))

        def report(name: str, samples: list[float]) -> None:
            print(f"  {name:10s} min={min(samples)*1000:8.1f} ms  median={statistics.median(samples)*1000:8.1f} ms")

        pixels = args.width * height
        print(f"ORBYX CPU benchmark — image {args.width}x{height} ({pixels/1e6:.2f} Mpx), "
              f"{args.iterations} iterations, CPU backend")
        report("calibrate", calibrate_times)
        report("generate", generate_times)
        print("NOTE: real CPU wall-clock on this host; not a GPU comparison or real-acquisition benchmark.")
        return 0
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
