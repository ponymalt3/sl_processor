"""
cocotb tests for wb_ixs — translated from wb_interconnect_tb.vhd.

Slaves are VHDL processes inside the wrapper (no Python slave model needed).
Verification is done by write-then-read-back through the masters.
"""

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import RisingEdge, Timer
from wb_master import WbMaster

CLK_PERIOD_NS = 20


def _create_masters(dut):
    cocotb.start_soon(Clock(dut.clk_i, CLK_PERIOD_NS, unit="ns").start())
    masters = [WbMaster(dut, f"m{i}_") for i in range(3)]
    for m in masters:
        m.idle()
    return masters


async def _setup(dut):
    masters = _create_masters(dut)
    dut.reset_n_i.value = 0
    for m in masters:
        m.idle()
    await Timer(33, unit="ns")
    dut.reset_n_i.value = 1
    await Timer(200, unit="ns")
    return masters


@cocotb.test()
async def test_slave_boundaries(dut):
    """Write to boundary addresses of each slave and read back."""
    masters = await _setup(dut)
    m0 = masters[0]

    await m0.write(0, 0x0000_BEEF)
    await m0.write(9, 0x0000_BEE0)
    val = await m0.read(0)
    assert val == 0x0000_BEEF, f"slv1[0] read-back failed: got 0x{val:08X}"
    assert await m0.read(9) == 0x0000_BEE0, "slv1[9] read-back failed"

    await m0.write(128, 0x0000_AA99)
    await m0.write(137, 0x0000_BB77)
    assert await m0.read(128) == 0x0000_AA99, "mem0[128] read-back failed"
    assert await m0.read(137) == 0x0000_BB77, "mem0[137] read-back failed"

    await m0.write(256, 0x0000_DDAB)
    await m0.write(265, 0x0000_BE88)
    assert await m0.read(256) == 0x0000_DDAB, "mem2[256] read-back failed"
    assert await m0.read(265) == 0x0000_BE88, "mem2[265] read-back failed"


@cocotb.test()
async def test_master_slave_access(dut):
    """Each master writes through its allowed slaves, then read back via m0."""
    masters = await _setup(dut)
    m0, m1, m2 = masters

    # m0 can access all three slaves
    await m0.write(1, 0x0000_ABCD)
    await m0.write(129, 0x0000_DABC)
    await m0.write(257, 0x0000_CDAB)

    # m1 can only access mem0
    await m1.write(130, 0x0000_1234)

    # m2 can access mem0 and mem2
    await m2.write(131, 0x0000_5678)
    await m2.write(259, 0x0000_9ABC)

    # Read back everything through m0
    assert await m0.read(1)   == 0x0000_ABCD, "m0→slv1 read-back"
    assert await m0.read(129) == 0x0000_DABC, "m0→mem0[129] read-back"
    assert await m0.read(257) == 0x0000_CDAB, "m0→mem2[257] read-back"
    assert await m0.read(130) == 0x0000_1234, "m1→mem0[130] read-back"
    assert await m0.read(131) == 0x0000_5678, "m2→mem0[131] read-back"
    assert await m0.read(259) == 0x0000_9ABC, "m2→mem2[259] read-back"


@cocotb.test()
async def test_parallel_different_slaves(dut):
    """3 masters write to 3 different slaves simultaneously."""
    masters = await _setup(dut)
    m0, m1, m2 = masters

    t0 = cocotb.start_soon(m0.write(4, 0x0000_B123))
    t1 = cocotb.start_soon(m1.write(132, 0x0000_C123))
    t2 = cocotb.start_soon(m2.write(260, 0x0000_D123))
    await t0
    await t1
    await t2

    assert await m0.read(4)   == 0x0000_B123, "parallel slv1"
    assert await m0.read(132) == 0x0000_C123, "parallel mem0"
    assert await m0.read(260) == 0x0000_D123, "parallel mem2"


@cocotb.test()
async def test_sequential_arbitration(dut):
    """3 masters compete for mem0 — arbiter serialises them."""
    masters = await _setup(dut)
    m0, m1, m2 = masters

    t0 = cocotb.start_soon(m0.write(133, 0x0000_B999))
    t1 = cocotb.start_soon(m1.write(134, 0x0000_C999))
    t2 = cocotb.start_soon(m2.write(135, 0x0000_D999))
    await t0
    await t1
    await t2

    assert await m0.read(133) == 0x0000_B999, "arbitration m0"
    assert await m0.read(134) == 0x0000_C999, "arbitration m1"
    assert await m0.read(135) == 0x0000_D999, "arbitration m2"


@cocotb.test()
async def test_burst_write_read(dut):
    """Burst-write 8 words into mem0, then burst-read them back."""
    masters = await _setup(dut)
    m0 = masters[0]

    write_data = [i for i in range(8)]
    await m0.write_burst(130, write_data)
    read_data = await m0.read_burst(128, 8)

    # Burst wraps lower 3 address bits: write at 130 → locals 2,3,4,5,6,7,0,1
    expected = [write_data[6], write_data[7],
                write_data[0], write_data[1], write_data[2], write_data[3],
                write_data[4], write_data[5]]
    assert read_data == expected, f"burst mismatch: got {read_data}, expected {expected}"
