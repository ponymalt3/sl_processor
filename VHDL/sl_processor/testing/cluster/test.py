"""sl_cluster cocotb test: embarrassingly-parallel 4-core smoke test.

Each core reads its own disjoint input word from ext_mem and writes
result = input*2 + coreIndex to its own disjoint output word (see
prog_resolved). No cross-core synchronization is exercised here -- this
test only proves the new cocotb/GHDL cluster infrastructure itself (code
preload, per-core reset/enable, wishbone arbitration between the probe and
the DUT's own code fetch) works, mirroring the scenario already hand-driven
in the plain VHDL testbench sl_cluster_tb.vhd.
"""

import os
from pathlib import Path

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import RisingEdge, Timer

from wb_master import WbMaster

CLK_PERIOD_NS = 20
CODE_MEM_BASE = 0
EXT_MEM_BASE  = 2048  # CodeMemSizeInKB*1024/4, see wrapper.vhd

# input=5,7,9,11 for cores 0..3; expected = input*2 + coreIndex (qfp32 raw hex)
INPUTS   = [0x05000000, 0x07000000, 0x09000000, 0x0B000000]
EXPECTED = [0x0A000000, 0x0F000000, 0x14000000, 0x19000000]


def _load_code() -> list:
    path = Path(os.environ["CLUSTER_CODE_HEX"])
    return [int(line, 16) for line in path.read_text().splitlines() if line.strip()]


@cocotb.test()
async def four_cores_run_in_parallel(dut):
    cocotb.start_soon(Clock(dut.clk_i, CLK_PERIOD_NS, unit="ns").start())

    probe = WbMaster(dut, "probe_")
    probe.idle()

    dut.reset_n_i.value = 0
    dut.core_en_i.value = 0b0000
    dut.core_reset_n_i.value = 0b1111
    await Timer(33, unit="ns")
    await RisingEdge(dut.clk_i)
    dut.reset_n_i.value = 1

    # code_mem is word-addressed with 2 packed 16-bit instructions/word (low
    # halfword first), matching sl_cluster's wb_cache addr_i(15 downto 1)/mux
    code = _load_code()
    for i in range(0, len(code), 2):
        lo = code[i]
        hi = code[i + 1] if i + 1 < len(code) else 0
        await probe.write(CODE_MEM_BASE + i // 2, lo | (hi << 16))

    for core, value in enumerate(INPUTS):
        await probe.write(EXT_MEM_BASE + core * 2, value)

    await Timer(100, unit="ns")

    # full reset cycle before enabling, mirrors sl_cluster_tb.vhd
    dut.reset_n_i.value = 0
    await Timer(23, unit="ns")
    dut.reset_n_i.value = 1

    await Timer(1300, unit="ns")  # let the instruction caches settle (64-line tag init = 64 cycles)

    dut.core_reset_n_i.value = 0b0000
    await RisingEdge(dut.clk_i)
    await RisingEdge(dut.clk_i)
    dut.core_reset_n_i.value = 0b1111

    await Timer(75, unit="ns")
    dut.core_en_i.value = 0b1111

    await Timer(8000, unit="ns")  # real 4-way bus contention is slower than single-core

    for core, expected in enumerate(EXPECTED):
        result = await probe.read(EXT_MEM_BASE + core * 2 + 1)
        assert result == expected, (
            f"core {core}: expected {expected:#010x}, got {result:#010x}"
        )
