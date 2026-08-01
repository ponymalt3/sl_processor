"""sl_system top-entity cocotb test.

Drives the whole board-level design purely over the UART debug protocol,
same as real hardware bring-up: load code+data into SDRAM, enable cores,
read results back. Exercises the full cache hierarchy end to end -- the
code fetch and per-core data-cache read/write on the way in, the L2/SDRAM
cache underneath both, and (the real point of this test) bus snooping:
core0 caches a data line, core1 writes the same address, and core0's
poll loop only sees the new value if the WriteThrough snoop path in
wb_cache.vhd actually invalidated its stale copy.
"""

import os
from pathlib import Path

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import Timer

from sdram_model import SdramModel
from wb_debug_ctrl import DebugCtrl
from run import GENERICS

CLK_PERIOD_NS = 20
CODE_MEM_WORDS = (GENERICS["CodeMemSizeInKB"] * 1024) // 4

# relative (ext-mem-local) addresses used by prog.rt's main1/main2
REL_DATA = 10
REL_RES0 = 20
REL_RES1 = 21


def _load_code() -> list:
    path = Path(os.environ["SYSTEM_CODE_HEX"])
    return [int(line, 16) for line in path.read_text().splitlines() if line.strip()]


@cocotb.test(timeout_time=2, timeout_unit="ms")
async def uart_load_run_and_snoop(dut):
    cocotb.start_soon(Clock(dut.clk_i, CLK_PERIOD_NS, unit="ns").start())

    sdram = SdramModel(dut)
    sdram.start()

    debug = DebugCtrl(dut, GENERICS["ClockFreqHz"], GENERICS["BaudRate"])

    dut.reset_n_i.value = 0
    await Timer(200, unit="ns")
    dut.reset_n_i.value = 1
    await Timer(200, unit="ns")


    assert await debug.ping(), "debug controller did not respond to PING"

    # sanity: raw write/readback through the L2/SDRAM cache, no cores involved
    ext_mem_words = (GENERICS["ExtMemSizeInKB"] * 1024) // 4
    sync_addr = ext_mem_words + 5
    assert await debug.write(sync_addr, [0xCAFEF00D])
    assert (await debug.read(sync_addr, 1)) == [0xCAFEF00D]

    sanity_addr = CODE_MEM_WORDS + 100
    assert await debug.write(sanity_addr, [0xDEADBEEF])
    await Timer(150_000, unit="ns")
    assert (await debug.read(sanity_addr, 1)) == [0xDEADBEEF]

    # load code: 2 packed 16-bit instructions per 32-bit code_mem word,
    # low halfword first (matches sl_cluster's code_addr(0) mux)
    code = _load_code()
    packed = []
    for i in range(0, len(code), 2):
        lo = code[i]
        hi = code[i + 1] if i + 1 < len(code) else 0
        packed.append(lo | (hi << 16))
    assert await debug.write(0, packed)

    await Timer(150_000, unit="ns")
    for i in (0, 1, len(packed) // 2, len(packed) - 1):
        got = (await debug.read(i, 1))[0]
        assert got == packed[i], (
            f"code readback mismatch at word {i}: expected {packed[i]:#010x}, got {got:#010x} -- "
            "the loaded code image is corrupted in memory before any core ever fetches it"
        )

    # enable cores 0 (main1: reader/poller) and 1 (main2: delayed writer);
    # 2 and 3 stay disabled
    assert await debug.set_cores(enable_mask=0b0011)

    await Timer(200_000, unit="ns")  # cores run to completion

    res0, res1 = await debug.read(CODE_MEM_WORDS + REL_RES0, 2)

    # every RT-language numeric literal compiles to qfp32 (see __qfp32(int32_t)
    # in VHDL/qfp32/qfp32.h), never a plain integer -- for a small whole
    # number like 1, that's exp=0, mant=1<<24, so its raw 32-bit encoding is
    # 1<<24, not 1 itself. Same convention as every C++ test's qfp32_t(N).toRaw().
    qfp32_one = 1 << 24

    assert res0 == 0, f"first read (before core1's write) expected 0, got {res0}"
    assert res1 == qfp32_one, (
        f"second read (after core1's write) expected {qfp32_one:#010x} (qfp32 1.0), got {res1:#010x} -- "
        "bus snooping did not invalidate core0's cached line"
    )
