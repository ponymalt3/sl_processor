"""Shared cocotb runner helper: one simulator run per test case.

GHDL writes exactly one waveform per simulator run (its runner hardcodes the
name to <toplevel>.ghw), so a waveform *per test case* means running the
simulator once per test case -- `testcase=` plus an explicit --wave path.
Elaboration is not repeated, only the run itself, so the extra cost is the
simulator start-up per test.

Waves land in the waves/ directory next to the suite's test.py (each run.py
passes its own `wave_dir`), one <testcase>.ghw per test case. GHW is GHDL's
native format:
unlike VCD/FST it keeps the VHDL types intact, so records (wb_slave_ifc_t) and
enums (wb_cache's state_t, wb_sdram's state_t) stay readable in GTKWave.

With waves turned off this falls back to a single run over all test cases,
which is what CI wants -- no per-test start-up, no waveform files.
"""

import importlib
import os
import sys
from pathlib import Path

# GHDL runtime option per format. Override with SIM_WAVE_FORMAT=fst|vcd|ghw.
WAVE_FORMATS = {"ghw": "--wave", "fst": "--fst", "vcd": "--vcd"}
WAVE_FORMAT = os.environ.get("SIM_WAVE_FORMAT", "ghw")


def discover_testcases(test_modules, search_dirs=()) -> list:
    """Names of the cocotb tests in `test_modules`, in declaration order.

    Imports the modules rather than parsing them: test cases may be registered
    dynamically (VHDL/qfp32/Test/test.py builds its per-operation tests in a
    loop and puts them into globals()), and only an import sees those.

    Returns [] if a module cannot be imported -- the caller then falls back to
    a single run, so a discovery problem never costs us the test itself.
    """
    if isinstance(test_modules, str):
        test_modules = [test_modules]

    added = [str(d) for d in search_dirs if str(d) not in sys.path]
    sys.path[:0] = added
    try:
        from cocotb._decorators import Test
    except ImportError:  # pragma: no cover - cocotb too old, fall back
        return []

    names = []
    try:
        for mod_name in test_modules:
            module = importlib.import_module(mod_name)
            names += [obj.name for obj in vars(module).values() if isinstance(obj, Test)]
    except Exception as exc:
        print(f"sim_runner: could not import {test_modules} ({type(exc).__name__}: {exc}), "
              "falling back to a single run with one waveform", file=sys.stderr)
        return []
    finally:
        for d in added:
            try:
                sys.path.remove(d)
            except ValueError:
                pass

    return names


def _safe_name(name: str) -> str:
    """Test name as a file name."""
    return "".join(c if (c.isalnum() or c in "._-") else "_" for c in name)


def run_tests(runner, *, test_module, build_dir, plusargs=(), waves=True,
              search_dirs=(), wave_dir=None, wave_name=None, **test_kwargs) -> None:
    """runner.test(), but once per test case with its own waveform.

    `wave_dir` overrides where the waveforms go (default <build_dir>/waves).
    `wave_name` overrides the file name -- for suites where the cocotb test
    case name says nothing, because the real test comes from a data file: the
    processor suite runs everything through one `test_from_vector` and passes
    the vector's own test name in here.

    Raises SystemExit if any test case failed, so CMake/CI still see a
    non-zero exit code.
    """
    build_dir = Path(build_dir)
    plusargs = list(plusargs)

    names = discover_testcases(test_module, search_dirs) if waves else []
    if not names:
        runner.test(test_module=test_module, build_dir=build_dir,
                    plusargs=plusargs, **test_kwargs)
        return

    wave_dir = Path(wave_dir) if wave_dir is not None else build_dir / "waves"
    wave_dir.mkdir(parents=True, exist_ok=True)

    from cocotb_tools.runner import get_results

    failed = []
    for name in names:
        if wave_name is None:
            stem = name
        elif len(names) == 1:
            stem = wave_name
        else:
            stem = f"{wave_name}.{name}"
        wave_file = wave_dir / f"{_safe_name(stem)}.{WAVE_FORMAT}"
        try:
            _dbg = os.environ.get("SIM_RUNNER_DEBUG", "")
            extra = {}
            if "no_testcase" not in _dbg:
                extra["testcase"] = [name]
            if "no_xml" not in _dbg:
                extra["results_xml"] = f"{name}.results.xml"
            _plus = list(plusargs)
            if "no_wavearg" not in _dbg:
                _plus.append(f"{WAVE_FORMATS[WAVE_FORMAT]}={wave_file}")
            results_xml = runner.test(
                test_module=test_module,
                build_dir=build_dir,
                plusargs=_plus,
                **extra,
                **test_kwargs)
            _, num_failed = get_results(results_xml)
        except (SystemExit, Exception) as exc:  # noqa: B014 - SystemExit is not an Exception
            print(f"sim_runner: {name} raised {type(exc).__name__}: {exc}", file=sys.stderr)
            num_failed = 1
        if num_failed:
            failed.append(name)

    print(f"\nsim_runner: {len(names) - len(failed)}/{len(names)} test cases passed, "
          f"waveforms in {wave_dir}")
    if failed:
        raise SystemExit("failed test cases: " + ", ".join(failed))
