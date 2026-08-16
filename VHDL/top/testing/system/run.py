#!/usr/bin/env python3
"""Cocotb simulation runner for the sl_system top-entity test (invoked by CMake)."""

import argparse
import os
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR  = Path(__file__).parent.resolve()            # system/
TOP_DIR     = SCRIPT_DIR.parent.parent                    # system/ -> testing/ -> top/
VHDL_ROOT   = TOP_DIR.parent                               # -> VHDL/
REPO_ROOT   = VHDL_ROOT.parent                              # -> SLProcessor/
QFP32_ROOT  = VHDL_ROOT / "qfp32"
SL_PROC_DIR = VHDL_ROOT / "sl_processor"
COCOTB_LIB  = VHDL_ROOT / "cocotb"

_DEFAULT_BUILD_DIR = REPO_ROOT / "build" / "sim" / "sim_system"

VHDL_SOURCES = [
    # qfp32 (dependency) -- order matters
    QFP32_ROOT / "qfp_p.vhd",
    QFP32_ROOT / "cla.vhd",
    QFP32_ROOT / "Units" / "misc.vhd",
    QFP32_ROOT / "Units" / "add.vhd",
    QFP32_ROOT / "Units" / "mul.vhd",
    QFP32_ROOT / "Units" / "divider.vhd",
    QFP32_ROOT / "Units" / "norm.vhd",
    QFP32_ROOT / "Units" / "recp.vhd",
    QFP32_ROOT / "Units" / "math.vhd",
    QFP32_ROOT / "Units" / "mac.vhd",
    QFP32_ROOT / "unit.vhd",
    # SL processor
    VHDL_ROOT   / "sl_misc.vhd",
    SL_PROC_DIR / "sl_structs_p.vhd",
    VHDL_ROOT   / "sl_dpram.vhd",
    SL_PROC_DIR / "sl_dec.vhd",
    SL_PROC_DIR / "sl_dec_ex.vhd",
    SL_PROC_DIR / "sl_execute.vhd",
    SL_PROC_DIR / "sl_state.vhd",
    SL_PROC_DIR / "sl_core.vhd",
    SL_PROC_DIR / "sl_control.vhd",
    SL_PROC_DIR / "sl_code_mem.vhd",
    SL_PROC_DIR / "sl_processor.vhd",
    # Wishbone
    VHDL_ROOT / "wishbone" / "wishbone_p.vhd",
    VHDL_ROOT / "wishbone" / "wb_master.vhd",
    VHDL_ROOT / "wishbone" / "wb_ixs_decode.vhd",
    VHDL_ROOT / "wishbone" / "wb_ixs_arbiter.vhd",
    VHDL_ROOT / "wishbone" / "wb_ixs.vhd",
    VHDL_ROOT / "wishbone" / "wb_cache.vhd",
    VHDL_ROOT / "wishbone" / "adapter" / "wb_cache_adapter.vhd",
    VHDL_ROOT / "wishbone" / "wb_mem.vhd",
    VHDL_ROOT / "wishbone" / "wb_sdram.vhd",
    VHDL_ROOT / "wishbone" / "adapter" / "wb_abp_bridge.vhd",
    # Cluster + top-level system
    SL_PROC_DIR / "sl_cluster.vhd",
    TOP_DIR / "uart.vhd",
    TOP_DIR / "wb_debug_ctrl.vhd",
    TOP_DIR / "top.vhd",
    SCRIPT_DIR / "wrapper.vhd",
]

GHDL_ARGS     = ["--std=08", "-frelaxed"]
GHDL_PLUSARGS = ["--ieee-asserts=disable"]

# Small generics keep the SDRAM model's address space and the sim runtime
# manageable; NarrowTag/BypassBaseAddr on the L1 caches (hardcoded in
# sl_cluster.vhd) stay far above this test's tiny address range regardless.
GENERICS = {
    "ClockFreqHz":        50_000_000,
    "BaudRate":            5_000_000,  # faster than real hardware, cuts sim time
    "LocalMemSizeInKB":    1,
    "CodeMemSizeInKB":     1,
    "ExtMemSizeInKB":      2,
    "CodeCacheSizeInKB":   1,
    "DataCacheSizeInKB":   1,
    "SdramCacheSizeInKB":  1,
    "SyncMemSizeInKB":     1,
}


def run(modules: list, build_dir: Path, gen_code_bin: Path, prog_source: Path, waves: bool = True):
    os.environ.setdefault("GHDL_BACKEND", "gcc")
    for p in (COCOTB_LIB, SCRIPT_DIR):
        if str(p) not in sys.path:
            sys.path.insert(0, str(p))

    build_dir.mkdir(parents=True, exist_ok=True)
    code_hex_path = build_dir / "code.hex"
    result = subprocess.run([str(gen_code_bin), str(prog_source)],
                             capture_output=True, text=True, check=True)
    code_hex_path.write_text(result.stdout)
    os.environ["SYSTEM_CODE_HEX"] = str(code_hex_path)

    from cocotb_tools.runner import get_runner
    from sim_runner import run_tests
    runner = get_runner("ghdl")

    runner.build(
        vhdl_sources=VHDL_SOURCES,
        build_args=GHDL_ARGS,
        hdl_library="work",
        hdl_toplevel="sl_system_tb_wrapper",
        build_dir=build_dir,
    )
    run_tests(
        runner,
        test_module=modules,
        hdl_toplevel="sl_system_tb_wrapper",
        hdl_toplevel_library="work",
        hdl_toplevel_lang="vhdl",
        build_dir=build_dir,
        parameters=GENERICS,
        test_args=GHDL_ARGS + [f"--workdir={build_dir}", f"-P{build_dir}"],
        # waveforms: one .ghw per test case in waves/ next to test.py, see
        # sim_runner.py -- .ghw keeps records (wb_slave_ifc_*_t) and enums
        # (wb_cache's state_t) intact, which VCD silently drops
        # ("$comment X is not handled $end").
        plusargs=GHDL_PLUSARGS,
        waves=waves,
        search_dirs=(COCOTB_LIB, SCRIPT_DIR),
        wave_dir=SCRIPT_DIR / "waves",
    )


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--modules", default="test")
    p.add_argument("--build-dir", default=str(_DEFAULT_BUILD_DIR))
    p.add_argument("--gen-code", required=True, help="path to the built gen_code executable")
    p.add_argument("--prog", default=str(SCRIPT_DIR / "prog.rt"),
                   help="RT-assembler source to compile for the 4 cores")
    p.add_argument("--no-waves", dest="waves", action="store_false")
    p.set_defaults(waves=True)
    args = p.parse_args()
    run(args.modules.split(","), Path(args.build_dir),
        Path(args.gen_code), Path(args.prog), args.waves)
