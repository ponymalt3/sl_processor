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
from wb_memory import WbMemoryModel

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


async def _reset(dut, l1s: list, probe: WbMaster, per_beat_stall_cycles: int = 0):
    """Bring the hierarchy up and start the L2's backing store.

    The store is a Python WbMemoryModel rather than a VHDL memory, so wait
    states are requested explicitly here. per_beat_stall_cycles > 0 models the
    real SDRAM controller, which closes the row between every word -- the
    combination of that pacing with genuine multi-master contention is what
    this bench exists for and what no other bench covers.
    """
    cocotb.start_soon(Clock(dut.clk_i, CLK_PERIOD_NS, unit="ns").start())
    mem = WbMemoryModel(dut, prefix="mem_", size=ADDR_SPACE, init_value=0,
                        per_beat_stall_cycles=per_beat_stall_cycles)
    dut.reset_n_i.value = 0
    for l1 in l1s:
        l1.idle()
    probe.idle()
    mem.idle()
    await Timer(33, unit="ns")
    await RisingEdge(dut.clk_i)
    dut.reset_n_i.value = 1
    mem.start()
    await Timer(500, unit="ns")
    return mem


@cocotb.test(timeout_time=5, timeout_unit="ms")
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


@cocotb.test(timeout_time=5, timeout_unit="ms")
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


@cocotb.test(timeout_time=5, timeout_unit="ms")
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


@cocotb.test(timeout_time=5, timeout_unit="ms")
async def minimal_same_line_thrash_during_sequential_write_repro(dut):
    """Minimal, fast (~11us vs. l2_eviction_under_concurrent_load's ~450us)
    version of the same eviction race. L2 has 16 lines x 8 words/line = 128
    words; address 0 and address 128 both map to physical L2 line 0
    (128 // 8 mod 16 == 0). L1_0 sequentially writes all 8 words of line 0
    (addr 0..7) while L1_1 concurrently, repeatedly writes a DIFFERENT tag of
    that same line (addr 128) throughout -- thrashing line 0 (evict +
    refetch) out from under L1_0's in-progress word-by-word sequence. The
    failure mode it guards against is a later word ending up with an earlier
    word's value, i.e. write-back/refetch corruption rather than a settle
    timing artifact."""
    await _check_same_line_thrash(dut)


async def _check_same_line_thrash(dut, per_beat_stall_cycles: int = 0):
    l1s = _all_l1(dut)
    probe = WbMaster(dut, "probe_")
    await _reset(dut, l1s, probe, per_beat_stall_cycles)

    # L1_0 sequentially writes all 8 words of line 0 (addr 0..7) while L1_1
    # concurrently, repeatedly, hammers a DIFFERENT tag of that same line
    # (addr 128) throughout -- thrashing line 0 out from under L1_0's
    # in-progress word-by-word sequence.
    values = [0xAAAA0000 + j for j in range(8)]

    async def thrash():
        for k in range(40):
            await l1s[1].write(128, 0xBBBB0000 + k)

    thrasher = cocotb.start_soon(thrash())
    for j in range(8):
        await l1s[0].write(j, values[j])
    await thrasher
    await Timer(500, unit="ns")

    for j in range(8):
        v = await probe.read(j)
        assert v == values[j], f"addr {j}: expected {values[j]:#010x}, got {v:#010x}"


@cocotb.test(timeout_time=5, timeout_unit="ms")
async def same_line_thrash_with_stalling_memory(dut):
    """The eviction race above, but with the backing store stalling before
    every beat like the real SDRAM controller. Contention and a slow memory
    together are what the hardware actually sees, and no other bench covers
    that combination: wb_cache's tests stall but have a single requester,
    this bench had contention but a store that could never stall."""
    await _check_same_line_thrash(dut, per_beat_stall_cycles=8)


@cocotb.test(timeout_time=5, timeout_unit="ms")
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
    await Timer(500, unit="ns")  # let every serialized write fully settle at L2

    for i in range(NUM_L1):
        v = await probe.read(base + i)
        assert v == values[i], f"addr {base+i}: expected {values[i]:#06x}, got {v:#010x}"


@cocotb.test(timeout_time=5, timeout_unit="ms")
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
    await Timer(500, unit="ns")

    for i in range(NUM_L1 // 2):
        v = await probe.read(base + NUM_L1 // 2 + i)
        assert v == 0x3000 + i, f"addr {base+NUM_L1//2+i}: got {v:#010x}"


@cocotb.test(timeout_time=5, timeout_unit="ms")
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
    # l1.write() returns once the L1's own WriteThrough pass-through accepts
    # the request (a posted write -- the cache doesn't block the requester on
    # the full bus round trip), not once it's durably visible at L2. Under
    # 8-way same-address contention the inner arbiter serializes all 8 one at
    # a time (confirmed via VCD: the last of the 8 doesn't actually commit at
    # L2 until ~1200ns in, while Combine() itself resolves around ~580ns).
    # 300ns wasn't enough settle margin and let probe.read() race ahead of
    # full serialization; give it enough headroom instead.
    await Timer(1500, unit="ns")

    final = await probe.read(addr)
    assert final in values, f"corrupted result {final:#010x}, not one of the 8 written values"

    for l1 in l1s:
        v = await l1.read(addr)
        assert v == final, f"L1[{l1.index}] disagrees with final value: {v:#010x} != {final:#010x}"


@cocotb.test(timeout_time=5, timeout_unit="ms")
async def l2_eviction_under_concurrent_load(dut):
    """Every L1 writes many more distinct lines than the L2 cache can hold
    (L2: 16 lines x 8 words = 128 words; this sweeps ~4x that), all
    concurrently, forcing continuous L2 eviction/writeback while still
    servicing other L1s' in-flight requests. Every value must still read
    back correctly afterwards."""
    await _check_l2_eviction_sweep(dut)


async def _check_l2_eviction_sweep(dut, per_beat_stall_cycles: int = 0,
                                   words_per_l1: int = None):
    l1s = _all_l1(dut)
    probe = WbMaster(dut, "probe_")
    await _reset(dut, l1s, probe, per_beat_stall_cycles)

    if words_per_l1 is None:
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
    await Timer(500, unit="ns")

    for addr, value in expected.items():
        v = await probe.read(addr)
        assert v == value, f"addr {addr}: expected {value:#010x}, got {v:#010x}"


@cocotb.test(timeout_time=20, timeout_unit="ms")
async def l2_eviction_under_concurrent_load_with_stalling_memory(dut):
    """The eviction sweep above against a backing store that stalls before
    every beat, i.e. continuous write-back and refill while the memory paces
    the bus the way the real SDRAM controller does. Each L1 sweeps a quarter
    of the full region so the run stays reasonably short; 8 x 32 = 256 words
    is still twice the L2's 128-word capacity, and L1_0's region collides
    with L1_4's on the same physical lines, so evictions are guaranteed."""
    await _check_l2_eviction_sweep(dut, per_beat_stall_cycles=8, words_per_l1=32)


@cocotb.test(timeout_time=5, timeout_unit="ms")
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
    await Timer(500, unit="ns")
    all_expected = {}
    for t in tasks:
        all_expected.update(t.result())

    for addr, value in all_expected.items():
        v = await probe.read(addr)
        assert v == value, f"final check addr {addr}: expected {value:#010x}, got {v:#010x}"
