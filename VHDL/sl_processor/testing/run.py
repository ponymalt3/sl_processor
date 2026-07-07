#!/usr/bin/env python3
"""Cocotb simulation runner for SL processor integration tests.

Single-worker mode:
  python3 run.py --test-vector path/to/test.vector

Parallel mode (default — compile once, then run each test via a worker pool):
  python3 run.py --test-vector path/to/test.vector [--workers N]

Re-run only failed tests from the previous run:
  python3 run.py --test-vector path/to/test.vector --failed-only
"""

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR   = Path(__file__).parent.resolve()
VHDL_ROOT    = SCRIPT_DIR.parent.parent                # testing/ → sl_processor/ → VHDL/
REPO_ROOT    = VHDL_ROOT.parent                        # → SLProcessor/
SL_PROC_DIR  = VHDL_ROOT / "sl_processor"
QFP32_ROOT   = VHDL_ROOT / "qfp32" / "trunk"
COCOTB_LIB   = VHDL_ROOT / "cocotb"

_DEFAULT_WORKERS   = int(os.environ.get("SIM_WORKERS", min(os.cpu_count() or 4, 8)))
_DEFAULT_BUILD_DIR = REPO_ROOT / "build" / "sim" / "sim_processor"

VHDL_SOURCES = [
    # qfp32 (dependency) — order matters
    QFP32_ROOT / "qfp_p.vhd",
    QFP32_ROOT / "cla.vhd",
    QFP32_ROOT / "Units" / "misc.vhd",
    QFP32_ROOT / "Units" / "add.vhd",
    QFP32_ROOT / "Units" / "mul.vhd",
    QFP32_ROOT / "Units" / "divider.vhd",
    QFP32_ROOT / "Units" / "norm.vhd",
    QFP32_ROOT / "Units" / "recp.vhd",
    QFP32_ROOT / "Units" / "math.vhd",
    QFP32_ROOT / "unit.vhd",
    # SL processor (sl_misc/sl_dpram stay at VHDL root)
    VHDL_ROOT    / "sl_misc.vhd",
    SL_PROC_DIR  / "sl_structs_p.vhd",
    VHDL_ROOT    / "sl_dpram.vhd",
    SL_PROC_DIR  / "sl_dec.vhd",
    SL_PROC_DIR  / "sl_dec_ex.vhd",
    SL_PROC_DIR  / "sl_execute.vhd",
    SL_PROC_DIR  / "sl_state.vhd",
    SL_PROC_DIR  / "sl_core.vhd",
    SL_PROC_DIR  / "sl_control.vhd",
    SL_PROC_DIR  / "sl_code_mem.vhd",
    SL_PROC_DIR  / "sl_processor.vhd",
    # Wishbone
    VHDL_ROOT / "wishbone" / "wishbone_p.vhd",
    VHDL_ROOT / "wishbone" / "wb_master.vhd",
    VHDL_ROOT / "wishbone" / "wb_ixs_decode.vhd",
    VHDL_ROOT / "wishbone" / "wb_ixs_arbiter.vhd",
    VHDL_ROOT / "wishbone" / "wb_ixs.vhd",
    # Xorshift peripheral
    VHDL_ROOT / "xorshift" / "wb_xorshift_slave.vhd",
    # Test wrapper
    SCRIPT_DIR / "wrapper.vhd",
]

GHDL_ARGS     = ["--std=08", "-frelaxed"]
GHDL_PLUSARGS = ["--ieee-asserts=disable"]

FAILED_VECTOR = SCRIPT_DIR / "failed.vector"


# ---------------------------------------------------------------------------
# Vector helpers
# ---------------------------------------------------------------------------

def split_vector(path: Path) -> list[list[str]]:
    tests: list[list[str]] = []
    current: list[str] = []
    with open(path) as f:
        for raw in f:
            line = raw.rstrip("\n")
            if not line:
                continue
            if int(line[:4], 16) == 0x0000 and current:
                tests.append(current)
                current = []
            current.append(line)
    if current:
        tests.append(current)
    return tests


def filter_tests(tests: list[list[str]], patterns: list[str]) -> list[list[str]]:
    if not patterns:
        return tests
    compiled = [re.compile(p) for p in patterns]
    return [t for t in tests if any(rx.search(t[0][5:]) for rx in compiled)]


def write_vector(tests: list[list[str]], path: Path) -> None:
    with open(path, "w") as f:
        for test in tests:
            for line in test:
                f.write(line + "\n")
        f.write("\n")


# ---------------------------------------------------------------------------
# Single-worker build + test
# ---------------------------------------------------------------------------

def _setup_env():
    os.environ.setdefault("GHDL_BACKEND", "gcc")
    for p in (COCOTB_LIB, SCRIPT_DIR):
        if str(p) not in sys.path:
            sys.path.insert(0, str(p))


def build(build_dir: Path) -> None:
    _setup_env()
    from cocotb_tools.runner import get_runner
    runner = get_runner("ghdl")
    runner.build(
        vhdl_sources=VHDL_SOURCES,
        build_args=GHDL_ARGS,
        hdl_library="work",
        hdl_toplevel="testing_wrapper",
        build_dir=build_dir,
    )


def test(modules: list[str], build_dir: Path, test_vector: Path,
         waves: bool = False, failed_vector: Path | None = None) -> None:
    _setup_env()
    os.environ["TEST_VECTOR"] = str(Path(test_vector).resolve())
    if failed_vector is not None:
        os.environ["FAILED_VECTOR"] = str(failed_vector)
    elif "FAILED_VECTOR" in os.environ:
        del os.environ["FAILED_VECTOR"]

    from cocotb_tools.runner import get_runner
    runner = get_runner("ghdl")
    runner.test(
        test_module=modules,
        hdl_toplevel="testing_wrapper",
        hdl_toplevel_library="work",
        hdl_toplevel_lang="vhdl",
        build_dir=build_dir,
        test_args=GHDL_ARGS + [f"--workdir={build_dir}", f"-P{build_dir}"],
        plusargs=GHDL_PLUSARGS + ([f"--fst={build_dir}/testing_wrapper.fst"] if waves else []),
        waves=False,
    )


def run(modules: list[str], build_dir: Path, test_vector: Path,
        waves: bool = False, failed_vector: Path | None = None) -> None:
    build(build_dir)
    test(modules, build_dir, test_vector, waves=waves, failed_vector=failed_vector)


# ---------------------------------------------------------------------------
# Parallel runner — dynamic worker pool, one task per test
# ---------------------------------------------------------------------------

def _run_single_test(args: tuple) -> tuple[str, bool, list[str], list[str]]:
    """Run exactly one test in its own GHDL session. Reuses compiled artifacts."""
    test_idx, test_lines, shared_build_dir, waves = args

    test_name = test_lines[0][5:].strip()
    task_dir  = shared_build_dir / "tasks" / f"{test_idx:04d}"
    task_dir.mkdir(parents=True, exist_ok=True)

    # Symlink compiled artifacts from the shared build dir so GHDL can elaborate
    # without recompiling. results.xml and FST go into the isolated task_dir.
    for src in shared_build_dir.iterdir():
        if src.is_file():
            dst = task_dir / src.name
            if not dst.exists() and not dst.is_symlink():
                dst.symlink_to(src.resolve())

    vec_path = task_dir / "test.vector"
    fv_path  = task_dir / "failed.vector"
    with open(vec_path, "w") as f:
        for line in test_lines:
            f.write(line + "\n")
    if fv_path.exists():
        fv_path.unlink()

    cmd = [
        sys.executable, str(SCRIPT_DIR / "run.py"),
        "--test-vector", str(vec_path),
        "--build-dir",   str(task_dir),
        "--failed-vector", str(fv_path),
        "--skip-build",
        "--workers", "1",
    ]
    if not waves:
        cmd.append("--no-waves")

    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        output = (proc.stdout + proc.stderr).splitlines()
        passed = proc.returncode == 0
        error_lines = [l.strip() for l in output
                       if "assertion failed" in l or "TIMEOUT" in l
                       or ("[ERROR]" in l and "cocotb" not in l.lower())]
        return (test_name, passed, error_lines, test_lines)
    except subprocess.TimeoutExpired:
        return (test_name, False, [f"TIMEOUT after 300s"], test_lines)


def run_parallel(modules: list[str], build_dir: Path, test_vector: Path,
                 n_workers: int, waves: bool = False) -> bool:
    from concurrent.futures import ProcessPoolExecutor, as_completed

    tests     = split_vector(test_vector)
    n_workers = min(n_workers, len(tests))

    print(f"Compiling VHDL...", end=" ", flush=True)
    build(build_dir)
    print("done", flush=True)
    print(f"Running {len(tests)} tests  [{n_workers} workers]", flush=True)

    task_args = [(i, test, build_dir, waves) for i, test in enumerate(tests)]

    all_ok            = True
    failed_test_lines: list[list[str]] = []
    n_done            = 0
    total             = len(tests)

    with ProcessPoolExecutor(max_workers=n_workers) as pool:
        futures = {pool.submit(_run_single_test, args): args for args in task_args}
        for fut in as_completed(futures):
            n_done += 1
            name, passed, errors, test_lines = fut.result()
            tag = "PASS" if passed else "FAIL"
            print(f"  [{n_done:3d}/{total}] {tag}  {name}", flush=True)
            if not passed:
                all_ok = False
                failed_test_lines.append(test_lines)
                for e in errors:
                    print(f"         {e}", flush=True)

    if failed_test_lines:
        write_vector(failed_test_lines, FAILED_VECTOR)
        print(f"\n{len(failed_test_lines)}/{total} test(s) failed."
              f"  Re-run with --failed-only to retry.")
    else:
        if FAILED_VECTOR.exists():
            FAILED_VECTOR.unlink()
        print(f"\nAll {total} tests passed!")

    return all_ok


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--modules", default="test")
    p.add_argument("--build-dir", default=str(_DEFAULT_BUILD_DIR))
    p.add_argument("--test-vector", required=True)
    p.add_argument("--failed-vector", default="")
    p.add_argument("--skip-build", action="store_true",
                   help="Skip VHDL compilation (reuse existing build artifacts)")
    p.add_argument("--workers", type=int, default=_DEFAULT_WORKERS,
                   help=f"Parallel simulation workers (default: {_DEFAULT_WORKERS})")
    p.add_argument("--failed-only", action="store_true",
                   help="Re-run only tests from failed.vector")
    p.add_argument("--filter", metavar="REGEX", action="append", default=[],
                   help="Whitelist regex for test names (repeatable)")
    p.add_argument("--no-waves", dest="waves", action="store_false")
    p.set_defaults(waves=False)
    args = p.parse_args()

    bd  = Path(args.build_dir)
    vec = Path(args.test_vector)

    if args.failed_only:
        if not FAILED_VECTOR.exists():
            print("No failed.vector found — nothing to re-run.")
            sys.exit(0)
        vec = FAILED_VECTOR

    mods = args.modules.split(",")

    if args.filter:
        all_tests = split_vector(vec)
        matched   = filter_tests(all_tests, args.filter)
        if not matched:
            print(f"No tests matched filters: {args.filter}")
            sys.exit(0)
        print(f"Filter matched {len(matched)}/{len(all_tests)} tests.")
        filtered_path = bd / "_filtered.vector"
        filtered_path.parent.mkdir(parents=True, exist_ok=True)
        write_vector(matched, filtered_path)
        vec = filtered_path

    if args.workers > 1:
        ok = run_parallel(mods, bd, vec, args.workers, args.waves)
        sys.exit(0 if ok else 1)
    else:
        fv = Path(args.failed_vector) if args.failed_vector else FAILED_VECTOR
        if not args.skip_build:
            build(bd)
        test(mods, bd, vec, args.waves, failed_vector=fv)
