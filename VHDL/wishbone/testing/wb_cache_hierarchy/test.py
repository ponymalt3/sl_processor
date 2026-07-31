"""Multi-level wb_cache hierarchy stress tests: 8 L1 write-through caches ->
nested wb_ixs arbiters -> a single L2 write-back cache -> backing memory,
mirroring sl_cluster.vhd + top.vhd's real nested topology (minus the
processor pipeline and real SDRAM). Exercises caching/snooping under many
different and genuinely-parallel access patterns to validate the caching
subsystem in isolation from the processor.
"""

import random

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import RisingEdge, Timer, Combine

from wb_master import WbMaster

CLK_PERIOD_NS = 20
NUM_L1 = 8
ADDR_SPACE = 1024  # matches SlaveSize in wrapper.vhd


class L1Client:
    """Drives one of the wrapper's flat l1_<i>_* cache ports directly."""

    def __init__(self, dut, index: int):
        self._clk = dut.clk_i
        prefix = f"l1_{index}_"
        self._addr     = getattr(dut, f"{prefix}addr_i")
        self._din      = getattr(dut, f"{prefix}din_i")
        self._dout     = getattr(dut, f"{prefix}dout_o")
        self._en       = getattr(dut, f"{prefix}en_i")
        self._we       = getattr(dut, f"{prefix}we_i")
        self._complete = getattr(dut, f"{prefix}complete_o")
        self.index = index

    def idle(self):
        self._addr.value = 0
        self._din.value  = 0
        self._en.value   = 0
        self._we.value   = 0

    async def read(self, addr: int) -> int:
        self._addr.value = addr
        self._en.value   = 1
        self._we.value   = 0
        await RisingEdge(self._clk)
        while self._complete.value != 1:
            await RisingEdge(self._clk)
        result = int(self._dout.value)
        await Timer(1, unit="ps")
        self._en.value = 0
        return result

    async def write(self, addr: int, data: int):
        self._addr.value = addr
        self._din.value  = data
        self._en.value   = 1
        self._we.value   = 1
        await RisingEdge(self._clk)
        while self._complete.value != 1:
            await RisingEdge(self._clk)
        await Timer(1, unit="ps")
        self._en.value = 0
        self._we.value = 0


def _all_l1(dut) -> list:
    return [L1Client(dut, i) for i in range(NUM_L1)]


async def _reset(dut, l1s: list, probe: WbMaster):
    cocotb.start_soon(Clock(dut.clk_i, CLK_PERIOD_NS, unit="ns").start())
    dut.reset_n_i.value = 0
    for l1 in l1s:
        l1.idle()
    probe.idle()
    await Timer(33, unit="ns")
    await RisingEdge(dut.clk_i)
    dut.reset_n_i.value = 1
    await Timer(500, unit="ns")


@cocotb.test()
async def basic_roundtrip_through_full_hierarchy(dut):
    """A single L1's write must survive through the inner IXS, L2, and
    backing memory, and read back correctly -- the baseline sanity check
    for the whole nested topology before stressing it."""
    l1s = _all_l1(dut)
    probe = WbMaster(dut, "probe_")
    await _reset(dut, l1s, probe)

    await l1s[3].write(100, 0xCAFEF00D)
    result = await l1s[3].read(100)
    assert result == 0xCAFEF00D, f"got {result:#010x}"

    # also visible from the direct probe path through the outer IXS
    probe_result = await probe.read(100)
    assert probe_result == 0xCAFEF00D, f"probe saw {probe_result:#010x}"


@cocotb.test()
async def snoop_invalidates_all_other_l1_caches(dut):
    """Two L1s cache the same address (read-hit path); a third L1 writes a
    new value. Both original caching L1s must observe the new value on
    their next read -- proving the snoop broadcast from the L2 write
    reaches every L1, not just a directly-adjacent one."""
    l1s = _all_l1(dut)
    probe = WbMaster(dut, "probe_")
    await _reset(dut, l1s, probe)

    addr = 200
    await l1s[0].write(addr, 0x11111111)

    # cache the line in two OTHER L1s via a read
    for reader in (l1s[1], l1s[5]):
        v = await reader.read(addr)
        assert v == 0x11111111

    await l1s[2].write(addr, 0x22222222)
    await Timer(200, unit="ns")  # let the snoop invalidation propagate

    for reader in (l1s[1], l1s[5]):
        v = await reader.read(addr)
        assert v == 0x22222222, (
            f"L1[{reader.index}] still saw stale data after another L1's "
            f"write -- snoop invalidation didn't reach it: {v:#010x}"
        )


@cocotb.test()
async def probe_write_is_visible_to_l1_via_snoop(dut):
    """A write from the direct probe path (mirrors wb_debug_ctrl.vhd) must
    also invalidate L1 caches, not just other-L1-originated writes."""
    l1s = _all_l1(dut)
    probe = WbMaster(dut, "probe_")
    await _reset(dut, l1s, probe)

    addr = 300
    await l1s[4].write(addr, 0xAAAAAAAA)
    assert await l1s[4].read(addr) == 0xAAAAAAAA

    await probe.write(addr, 0xBBBBBBBB)
    await Timer(200, unit="ns")

    v = await l1s[4].read(addr)
    assert v == 0xBBBBBBBB, f"L1 didn't see the probe's write: {v:#010x}"


@cocotb.test()
async def parallel_writes_to_disjoint_addresses_all_land(dut):
    """All 8 L1s fire a write on the same cycle, each to its own disjoint
    address -- real concurrency through the 8-way inner arbiter, not
    sequential access. None may be lost or land at the wrong address."""
    l1s = _all_l1(dut)
    probe = WbMaster(dut, "probe_")
    await _reset(dut, l1s, probe)

    base = 400
    values = [0x1000 + i for i in range(NUM_L1)]

    await Combine(*[cocotb.start_soon(l1s[i].write(base + i, values[i])) for i in range(NUM_L1)])

    for i in range(NUM_L1):
        v = await probe.read(base + i)
        assert v == values[i], f"addr {base+i}: expected {values[i]:#06x}, got {v:#010x}"


@cocotb.test()
async def parallel_read_and_write_to_disjoint_addresses(dut):
    """Half the L1s write while the other half concurrently read DIFFERENT
    addresses -- mixed concurrent traffic through the same arbiters."""
    l1s = _all_l1(dut)
    probe = WbMaster(dut, "probe_")
    await _reset(dut, l1s, probe)

    base = 500
    for i in range(NUM_L1 // 2):
        await probe.write(base + i, 0x2000 + i)

    async def do_write(l1, addr, value):
        await l1.write(addr, value)

    async def do_read(l1, addr, expected):
        v = await l1.read(addr)
        assert v == expected, f"L1[{l1.index}] addr {addr}: expected {expected:#06x}, got {v:#010x}"

    tasks = []
    for i in range(NUM_L1 // 2):
        tasks.append(cocotb.start_soon(do_write(l1s[i], base + NUM_L1 // 2 + i, 0x3000 + i)))
    for i in range(NUM_L1 // 2):
        tasks.append(cocotb.start_soon(do_read(l1s[NUM_L1 // 2 + i], base + i, 0x2000 + i)))
    await Combine(*tasks)

    for i in range(NUM_L1 // 2):
        v = await probe.read(base + NUM_L1 // 2 + i)
        assert v == 0x3000 + i, f"addr {base+NUM_L1//2+i}: got {v:#010x}"


@cocotb.test()
async def concurrent_same_address_writes_do_not_corrupt(dut):
    """All 8 L1s race to write the SAME address concurrently. Wishbone/IXS
    arbitration serializes them one at a time -- the final value must be
    EXACTLY one of the 8 written values (never a torn/mixed bit pattern),
    and every L1 must agree on it after snoop settles."""
    l1s = _all_l1(dut)
    probe = WbMaster(dut, "probe_")
    await _reset(dut, l1s, probe)

    addr = 600
    values = [0xA0000000 + i for i in range(NUM_L1)]

    await Combine(*[cocotb.start_soon(l1s[i].write(addr, values[i])) for i in range(NUM_L1)])
    await Timer(300, unit="ns")

    final = await probe.read(addr)
    assert final in values, f"corrupted result {final:#010x}, not one of the 8 written values"

    for l1 in l1s:
        v = await l1.read(addr)
        assert v == final, f"L1[{l1.index}] disagrees with final value: {v:#010x} != {final:#010x}"


@cocotb.test()
async def l2_eviction_under_concurrent_load(dut):
    """Every L1 writes many more distinct lines than the L2 cache can hold
    (L2: 16 lines x 8 words = 128 words; this sweeps ~4x that), all
    concurrently, forcing continuous L2 eviction/writeback while still
    servicing other L1s' in-flight requests. Every value must still read
    back correctly afterwards."""
    l1s = _all_l1(dut)
    probe = WbMaster(dut, "probe_")
    await _reset(dut, l1s, probe)

    words_per_l1 = ADDR_SPACE // NUM_L1  # 128, disjoint per-L1 region
    expected = {}

    async def sweep(l1: L1Client):
        base = l1.index * words_per_l1
        for j in range(words_per_l1):
            addr = base + j
            value = (l1.index << 24) | j
            await l1.write(addr, value)
            expected[addr] = value

    await Combine(*[cocotb.start_soon(sweep(l1)) for l1 in l1s])

    for addr, value in expected.items():
        v = await probe.read(addr)
        assert v == value, f"addr {addr}: expected {value:#010x}, got {v:#010x}"


@cocotb.test()
async def random_concurrent_traffic_stress(dut):
    """Several workers hammer randomly-chosen (but per-worker-disjoint)
    addresses with random read/write traffic concurrently for many
    iterations -- a broad, unscripted stress pass over the whole nested
    cache/arbiter/snoop path."""
    random.seed(1234)
    l1s = _all_l1(dut)
    probe = WbMaster(dut, "probe_")
    await _reset(dut, l1s, probe)

    iterations = 40
    region_size = ADDR_SPACE // NUM_L1

    async def worker(l1: L1Client):
        base = l1.index * region_size
        local_expected = {}
        for _ in range(iterations):
            addr = base + random.randrange(region_size)
            if addr not in local_expected or random.random() < 0.7:
                value = random.randrange(0, 2**32)
                await l1.write(addr, value)
                local_expected[addr] = value
            else:
                v = await l1.read(addr)
                assert v == local_expected[addr], (
                    f"L1[{l1.index}] addr {addr}: expected {local_expected[addr]:#010x}, got {v:#010x}"
                )
        return local_expected

    tasks = [cocotb.start_soon(worker(l1)) for l1 in l1s]
    await Combine(*tasks)
    all_expected = {}
    for t in tasks:
        all_expected.update(t.result())

    for addr, value in all_expected.items():
        v = await probe.read(addr)
        assert v == value, f"final check addr {addr}: expected {value:#010x}, got {v:#010x}"
