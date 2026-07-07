"""wb_cache cocotb tests: write-back, write-through, and stress."""

import random
import cocotb
from cocotb.triggers import Timer
from wb_cache import WriteBackCache, WriteThroughCache


# ---------------------------------------------------------------------------
# Write-back tests
# ---------------------------------------------------------------------------

def _setup_wb(dut):
    WriteBackCache.start_clock(dut)
    cache = WriteBackCache(dut)
    cache.start()
    return cache


@cocotb.test()
async def simple_read_with_fetch(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    result = await cache.read(256)
    assert result == 0xFFFF_FFFF, f"expected 0xFFFFFFFF, got {result:#010x}"


@cocotb.test()
async def cached_write_to_existing_line(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    await cache.write(257, 0xABCDEF00)
    await cache.flush_line(256)

    assert cache.mem[257] == 0xABCDEF00, "cache line not written back to memory"
    assert await cache.read(257) == 0xABCDEF00, "stale data after writeback"


@cocotb.test()
async def write_to_invalid_cache_line(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    t0 = cache.sim_time_ns()
    await cache.write(27, 0xABABABAB)
    result = await cache.read(27)
    elapsed = cache.sim_time_ns() - t0

    assert elapsed <= 240, f"fetch took {elapsed:.0f}ns, expected ≤240ns (≤12 cycles)"
    assert cache.mem[27] == 0xFFFF_FFFF, "premature writeback before flush"
    assert result == 0xABABABAB, f"wrong read-back: {result:#010x}"

    await cache.flush_line(27)
    assert cache.mem[27] == 0xABABABAB, "data not written back after flush"


@cocotb.test()
async def write_to_existing_dirty_cache_line(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    await cache.write(26, 0xCCCCCCAB)

    t0 = cache.sim_time_ns()
    await cache.write(90, 0xDEADBEEF)
    result = await cache.read(90)
    elapsed = cache.sim_time_ns() - t0

    assert elapsed <= 440, f"fetch after eviction took {elapsed:.0f}ns (writeback+fetch, ≤22 cycles)"
    assert cache.mem[26] == 0xCCCCCCAB, "dirty line not written back on eviction"
    assert result == 0xDEADBEEF, f"wrong data in new line: {result:#010x}"

    await cache.flush_line(90)
    assert cache.mem[90] == 0xDEADBEEF, "new line not written back"


@cocotb.test()
async def write_is_delayed_correctly(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    await cache.write(89, 0x1234BEAD)
    await cache.write(24, 0xDEADBE3F)
    await cache.write(27, 0xDEADB33F)
    result = await cache.read(27)

    assert cache.mem[89] == 0x1234BEAD, "line 89 not written back"
    assert result == 0xDEADB33F, f"wrong data: {result:#010x}"

    await cache.flush_line(27)
    assert cache.mem[24] == 0xDEADBE3F and cache.mem[27] == 0xDEADB33F, \
        "delayed writes not flushed correctly"


@cocotb.test()
async def write_with_stall_while_pending_fetch(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    await cache.write(1,  0x00DEAD00)
    await cache.write(6,  0xCAFEBABE)
    await cache.write(64, 0xDEADB00B)
    await cache.write(68, 0xB00BDEAD)

    assert await cache.read(64) == 0xDEADB00B, "line 64 wrong"
    assert await cache.read(68) == 0xB00BDEAD, "line 68 wrong"

    await cache.flush_line(64)
    await cache.flush_line(68)

    assert cache.mem[64] == 0xDEADB00B and cache.mem[68] == 0xB00BDEAD, \
        "writeback incorrect"


@cocotb.test()
async def pending_fetch_does_not_stall_read(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    await cache.write(9, 0x9987_6543)
    await cache.flush_line(0)
    await cache.read(0)

    t0 = cache.sim_time_ns()
    result = await cache.read(9)
    elapsed = cache.sim_time_ns() - t0

    assert elapsed <= 20, f"read stalled by pending fetch ({elapsed:.0f}ns)"
    assert result == 0x9987_6543, f"wrong data: {result:#010x}"


@cocotb.test()
async def pending_fetch_does_not_stall_write(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    await cache.read(18)
    await cache.flush_line(13)
    await cache.read(13)

    t0 = cache.sim_time_ns()
    await cache.write(18, 0x9987_EEEE)
    elapsed = cache.sim_time_ns() - t0

    assert elapsed <= 40, f"write stalled by pending fetch ({elapsed:.0f}ns)"
    assert await cache.read(18) == 0x9987_EEEE


@cocotb.test()
async def pending_writeback_does_not_stall_read(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    await cache.write(35, 0xDDEADBEE)
    await cache.write(37, 0xDEADBEE2)
    await cache.read(37 + 64)

    t0 = cache.sim_time_ns()
    result = await cache.read(35)
    elapsed = cache.sim_time_ns() - t0

    assert elapsed <= 20, f"read stalled by pending writeback ({elapsed:.0f}ns)"
    assert result == 0xDDEADBEE, f"wrong data: {result:#010x}"


@cocotb.test()
async def pending_writeback_does_not_stall_write(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    await cache.read(48)
    await cache.write(45,      0xEADBBBEE)
    await cache.write(45 + 64, 0xDEDEDEDE)

    t0 = cache.sim_time_ns()
    await cache.write(48, 0xEADBBEE2)
    elapsed = cache.sim_time_ns() - t0

    assert elapsed <= 40, f"write stalled by pending writeback ({elapsed:.0f}ns)"
    assert await cache.read(48) == 0xEADBBEE2


@cocotb.test()
async def idle_write_only_takes_one_cycle(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    await cache.read(54)
    await Timer(200, unit="ns")

    t0 = cache.sim_time_ns()
    await cache.write(54, 0xBEDBED99)
    elapsed = cache.sim_time_ns() - t0

    assert elapsed <= 20, f"idle write took {elapsed:.0f}ns, expected ≤20ns (1 cycle)"
    assert await cache.read(54) == 0xBEDBED99


# ---------------------------------------------------------------------------
# Write-through tests
# ---------------------------------------------------------------------------

def _setup_wt(dut):
    WriteThroughCache.start_clock(dut)
    cache = WriteThroughCache(dut)
    cache.start()
    return cache


@cocotb.test()
async def write_through_with_invalid_line(dut):
    cache = _setup_wt(dut)
    await cache.reset()

    t0 = cache.sim_time_ns()
    await cache.write(3, 0xABC0_0000)
    elapsed = cache.sim_time_ns() - t0

    assert elapsed <= 100, f"write took {elapsed:.0f}ns, expected ≤100ns"
    await Timer(20, unit="ns")
    assert cache.mem[3] == 0xABC0_0000, "write-through to memory failed"
    assert await cache.read(3) == 0xABC0_0000, "read-back mismatch"


@cocotb.test()
async def write_while_fetching_data(dut):
    cache = _setup_wt(dut)
    await cache.reset()

    await cache.write(7, 0xABCD_9999)
    await cache.flush_line(7)
    await cache.read(7)

    await cache.write(10, 0xABCD_7777)
    await Timer(20, unit="ns")

    assert cache.mem[10] == 0xABCD_7777, "write-through during fetch failed"
    assert await cache.read(10) == 0xABCD_7777


@cocotb.test()
async def write_through_to_same_line(dut):
    cache = _setup_wt(dut)
    await cache.reset()

    await cache.write(12, 0xA123_456B)
    await cache.write(13, 0xA123_456C)
    await Timer(20, unit="ns")

    assert cache.mem[12] == 0xA123_456B and cache.mem[13] == 0xA123_456C, \
        "write-through to same line failed"
    assert await cache.read(12) == 0xA123_456B
    assert await cache.read(13) == 0xA123_456C


@cocotb.test()
async def write_through_to_different_lines(dut):
    cache = _setup_wt(dut)
    await cache.reset()

    await cache.write(14, 0xA123_456D)
    await cache.write(16, 0xA123_456E)
    await Timer(20, unit="ns")

    assert cache.mem[14] == 0xA123_456D and cache.mem[16] == 0xA123_456E, \
        "write-through to different lines failed"
    assert await cache.read(14) == 0xA123_456D
    assert await cache.read(16) == 0xA123_456E


@cocotb.test()
async def write_through_followed_by_fetched_read(dut):
    cache = _setup_wt(dut)
    await cache.reset()

    await cache.read(21)
    await cache.write(22, 0xA123_456F)

    t0 = cache.sim_time_ns()
    result = await cache.read(22)
    elapsed = cache.sim_time_ns() - t0

    assert elapsed <= 20, f"read after write took {elapsed:.0f}ns"
    assert result == 0xA123_456F, f"stale data: {result:#010x}"
    assert cache.mem[22] == 0xA123_456F, "write-through to memory failed"


@cocotb.test()
async def write_through_followed_by_idle_read(dut):
    cache = _setup_wt(dut)
    await cache.reset()

    await cache.write(27, 0xA123_B123)
    await Timer(60, unit="ns")

    t0 = cache.sim_time_ns()
    result = await cache.read(27)
    elapsed = cache.sim_time_ns() - t0

    assert elapsed <= 120, f"read after idle took {elapsed:.0f}ns"
    assert result == 0xA123_B123
    assert cache.mem[27] == 0xA123_B123, "write-through to memory failed"


@cocotb.test()
async def invalidate_while_idle(dut):
    cache = _setup_wt(dut)
    await cache.reset()

    await cache.read(33)
    await cache.backdoor_write(33, 0xDCBA_9913)
    await Timer(100, unit="ns")
    await cache.invalidate(33)
    await Timer(40, unit="ns")

    assert await cache.read(33) == 0xDCBA_9913, \
        "cache not reloaded after idle invalidate"


@cocotb.test()
async def invalidate_while_fetching(dut):
    cache = _setup_wt(dut)
    await cache.reset()

    await cache.read(36)
    await cache.backdoor_write(36, 0xDC33_9913)
    await cache.invalidate(36)
    await Timer(40, unit="ns")

    assert await cache.read(36) == 0xDC33_9913, \
        "cache not reloaded after invalidate during fetch"


@cocotb.test()
async def invalidate_while_reading(dut):
    cache = _setup_wt(dut)
    await cache.reset()

    await cache.read(40)
    await cache.backdoor_write(40, 0xDCBB_BB00)

    cache._inv_addr.value = 40
    cache._inv_en.value   = 1
    await cache.read(41)
    cache._inv_en.value   = 0
    await Timer(40, unit="ns")

    assert await cache.read(40) == 0xDCBB_BB00, \
        "cache not reloaded after invalidate during read"


# ---------------------------------------------------------------------------
# Stress tests
# ---------------------------------------------------------------------------

SEED     = 0xDEAD_BEEF
MEM_SIZE = 256
N_READS  = 1024 * 4
N_WRITES = 512  * 4
N_INVS   = 433  * 4


async def _stress(cache, n_reads, n_writes, n_invalidates=0):
    rng = random.Random(SEED)
    ref = list(cache.mem[:MEM_SIZE])

    for addr in range(MEM_SIZE):
        ref[addr] = await cache.read(addr)

    r, w, inv = n_reads, n_writes, n_invalidates
    wdata = 0x0000_1111

    while r or w or inv:
        total = r + w + inv
        roll  = rng.randrange(total)

        if roll < r:
            addr   = rng.randrange(MEM_SIZE)
            result = await cache.read(addr)
            assert result == ref[addr], \
                f"read[{addr}]: got {result:#010x}, expected {ref[addr]:#010x}"
            r -= 1
        elif roll < r + w:
            addr = rng.randrange(MEM_SIZE)
            await cache.write(addr, wdata)
            ref[addr] = wdata
            wdata = (wdata + 1) & 0xFFFF_FFFF
            w -= 1
        else:
            addr = rng.randrange(MEM_SIZE)
            await cache.backdoor_write(addr, wdata)
            ref[addr] = wdata
            wdata = (wdata + 1) & 0xFFFF_FFFF
            await cache.invalidate(addr)
            inv -= 1

    for addr in range(MEM_SIZE):
        result = await cache.read(addr)
        assert result == ref[addr], \
            f"final check[{addr}]: got {result:#010x}, expected {ref[addr]:#010x}"


@cocotb.test()
async def write_back_random_access(dut):
    WriteBackCache.start_clock(dut)
    cache = WriteBackCache(dut)
    cache.start()
    await cache.reset()
    await _stress(cache, N_READS, N_WRITES)


@cocotb.test()
async def write_through_random_access(dut):
    WriteThroughCache.start_clock(dut)
    cache = WriteThroughCache(dut)
    cache.start()
    await cache.reset()
    await _stress(cache, N_READS, N_WRITES, N_INVS)
