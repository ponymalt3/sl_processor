#!/usr/bin/env python3
"""Cocotb simulation runner for wb_cache tests (invoked by CMake)."""

import argparse
import os
import sys
from pathlib import Path

SCRIPT_DIR         = Path(__file__).parent.resolve()
VHDL_ROOT          = SCRIPT_DIR.parent.parent.parent          # wb_cache/ → testing/ → wishbone/ → VHDL/
REPO_ROOT          = VHDL_ROOT.parent
COCOTB_LIB         = VHDL_ROOT / "cocotb"
_DEFAULT_BUILD_DIR = REPO_ROOT / "build" / "sim" / "sim_wb_cache"

VHDL_SOURCES = [
    VHDL_ROOT / "sl_misc.vhd",
    VHDL_ROOT / "sl_dpram.vhd",
    VHDL_ROOT / "wishbone" / "wishbone_p.vhd",
    VHDL_ROOT / "wishbone" / "wb_master.vhd",
    VHDL_ROOT / "wishbone" / "wb_cache.vhd",
    SCRIPT_DIR / "wrapper.vhd",
]

GHDL_ARGS      = ["--std=08", "-frelaxed"]
GHDL_PLUSARGS  = ["--ieee-asserts=disable"]


def run(modules: list, build_dir: Path, waves: bool = False):
    os.environ.setdefault("GHDL_BACKEND", "gcc")
    for p in (COCOTB_LIB, SCRIPT_DIR):
        if str(p) not in sys.path:
            sys.path.insert(0, str(p))

    from cocotb_tools.runner import get_runner
    runner = get_runner("ghdl")

    runner.build(
        vhdl_sources=VHDL_SOURCES,
        build_args=GHDL_ARGS,
        hdl_library="work",
        hdl_toplevel="wb_cache_wrapper",
        build_dir=build_dir,
    )
    runner.test(
        test_module=modules,
        hdl_toplevel="wb_cache_wrapper",
        hdl_toplevel_library="work",
        hdl_toplevel_lang="vhdl",
        build_dir=build_dir,
        test_args=GHDL_ARGS + [f"--workdir={build_dir}", f"-P{build_dir}"],
        plusargs=GHDL_PLUSARGS + ([f"--fst={build_dir}/wb_cache_wrapper.fst"] if waves else []),
        waves=False,
    )


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--modules", default="test")
    p.add_argument("--build-dir", default=str(_DEFAULT_BUILD_DIR))
    p.add_argument("--no-waves", dest="waves", action="store_false")
    p.set_defaults(waves=False)
    args = p.parse_args()
    run(args.modules.split(","), Path(args.build_dir), args.waves)
