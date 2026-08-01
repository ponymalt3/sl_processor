"""wb_cache cocotb tests: write-back, write-through, and stress."""

import random
import cocotb
from cocotb.triggers import Timer
from wb_cache import (WriteBackCache, WriteThroughCache, BypassCache,
                      CacheThroughArbiter, NarrowTagCache)

# ---------------------------------------------------------------------------
# Write-back tests
# ---------------------------------------------------------------------------


def _setup_wb(dut):
    WriteBackCache.start_clock(dut)
    cache = WriteBackCache(dut)
    cache.start()
    return cache


def _setup_wb_stalling(dut, per_beat_stall_cycles):
    WriteBackCache.start_clock(dut)
    cache = WriteBackCache(dut, per_beat_stall_cycles=per_beat_stall_cycles)
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


# wrapper.vhd's write-back DUT: WordsPerLine=4, NumberOfLines=16 -> 64 words,
# so addr and addr+64 share a tag-index with a different tag.
WORDS_PER_LINE = 4
LINE_BASE = 8  # word 0 of a line (8 = 2*WORDS_PER_LINE), keeps the maths obvious


async def _check_fetch_fills_whole_line(cache, start_word: int):
    """Read one word of an uncached line, then verify EVERY word of that line.

    A read miss starts the burst at the requested word and wraps around inside
    the line (wb_master.vhd's `(adr and not mask) or ((adr+1) and mask)`), so
    for start_word=2 the words arrive 2,3,0,1. Every one of them has to land at
    its own address -- that is what mem1_addr's wrap and mem1_addr_ov_bit are
    for. Reading a non-zero word of a line first is the normal case, not a
    corner case: it happens on any miss that isn't aligned to a line start.
    """
    for i in range(WORDS_PER_LINE):
        cache.mem[LINE_BASE + i] = 0x1000 + i

    got = await cache.read(LINE_BASE + start_word)
    assert got == 0x1000 + start_word, (
        f"critical word (offset {start_word}) wrong: got {got:#010x}, "
        f"expected {0x1000 + start_word:#010x}"
    )

    for i in range(WORDS_PER_LINE):
        got = await cache.read(LINE_BASE + i)
        assert got == 0x1000 + i, (
            f"word at offset {i} wrong after a fetch that started at offset "
            f"{start_word}: got {got:#010x}, expected {0x1000 + i:#010x}"
        )


@cocotb.test()
async def fetch_starting_mid_line_fills_whole_line(dut):
    cache = _setup_wb(dut)
    await cache.reset()
    await _check_fetch_fills_whole_line(cache, start_word=2)


@cocotb.test(timeout_time=50, timeout_unit="us")
async def fetch_starting_mid_line_fills_whole_line_while_slave_stalls(dut):
    """Same as above against a slave that stalls before every beat, like the
    real SDRAM controller does (wb_sdram.vhd closes the row between words)."""
    cache = _setup_wb_stalling(dut, per_beat_stall_cycles=8)
    await cache.reset()
    await _check_fetch_fills_whole_line(cache, start_word=2)


async def _check_writeback_writes_whole_line(cache):
    """Dirty every word of a line, evict it, and verify EVERY word reached
    backing memory at its own address.

    Most other tests only check one or two words of an evicted line, so a
    write-back that gets the addressing right for some words and wrong for
    others survives them. Writing a whole line and then evicting it is what
    any real workload does as soon as it touches more than one word per line.
    """
    for i in range(WORDS_PER_LINE):
        await cache.write(LINE_BASE + i, 0x2000 + i)

    await cache.flush_line(LINE_BASE)

    for i in range(WORDS_PER_LINE):
        assert cache.mem[LINE_BASE + i] == 0x2000 + i, (
            f"word at offset {i} of the evicted line is wrong in backing "
            f"memory: got {cache.mem[LINE_BASE + i]:#010x}, "
            f"expected {0x2000 + i:#010x}"
        )


@cocotb.test()
async def writeback_writes_every_word_of_the_line(dut):
    cache = _setup_wb(dut)
    await cache.reset()
    await _check_writeback_writes_whole_line(cache)


@cocotb.test(timeout_time=50, timeout_unit="us")
async def writeback_writes_every_word_of_the_line_while_slave_stalls(dut):
    """Same as above against a per-beat-stalling slave -- the combination the
    suite was missing: a full-line write-back whose every word is checked,
    while the slave paces the burst the way real SDRAM does."""
    cache = _setup_wb_stalling(dut, per_beat_stall_cycles=8)
    await cache.reset()
    await _check_writeback_writes_whole_line(cache)


@cocotb.test(timeout_time=50, timeout_unit="us")
async def writeback_through_adapter_survives_per_beat_stalling_slave(dut):
    """Same eviction-under-a-per-beat-stalling-slave case as the test above,
    but reached through the ix_* path (wb_master -> wb_ixs ->
    wb_cache_adapter(IsConnectedToIXS=true) -> wb_cache), i.e. how
    top.vhd actually drives its L2. The test above pokes wb_cache's flat
    ports directly, so en_i/addr_i arrive undelayed; through the adapter
    they don't (`en_o <= req and req_1d`), which shifts when the cache's
    write-back burst sees stall/dready relative to its own request. This
    covers the combination neither existing test does: adapter-routed
    request AND a slave that stalls before every beat rather than only
    before the burst's first one.

    Unlike the flat-port test above, addresses must stay inside 0..255 here:
    the ix_ path goes through a real wb_ixs whose decoder maps only
    wb_slave("cache", 0, 256) (see wrapper.vhd), so anything above that is
    never routed to the cache at all. addr and addr+64 share a tag-index
    (4 words/line x 16 lines = 64 words) with different tags, so reading the
    latter evicts the former."""
    CacheThroughArbiter.start_clock(dut)
    cache = CacheThroughArbiter(dut, per_beat_stall_cycles=8)
    cache.start()
    await cache.reset()

    addr = 10
    await cache.write(addr, 0xABCDEF00)
    await cache.flush_line(addr)

    assert cache.mem[addr] == 0xABCDEF00, (
        f"cache line not written back correctly through the adapter under a "
        f"per-beat-stalling slave, got {cache.mem[addr]:#010x}"
    )
    assert await cache.read(addr) == 0xABCDEF00, "stale data after writeback"


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
async def hit_survives_after_arbiter_regrant(dut):
    CacheThroughArbiter.start_clock(dut)
    cache = CacheThroughArbiter(dut)
    cache.start()
    await cache.reset()

    await cache.write(100, 0xDEADBEEF)
    await Timer(2_000, unit="ns")  # matches the long idle gap in the system test

    result = await cache.read(100)
    assert result == 0xDEADBEEF, (
        f"wrong read-back: {result:#010x} -- a real cache hit through the "
        "arbiter was read back as a miss, and the spurious re-fetch "
        "clobbered the cached (not yet written back) data"
    )


@cocotb.test()
async def write_to_existing_dirty_cache_line(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    await cache.write(26, 0xCCCCCCAB)

    t0 = cache.sim_time_ns()
    await cache.write(90, 0xDEADBEEF)
    result = await cache.read(90)
    elapsed = cache.sim_time_ns() - t0

    assert (
        elapsed <= 440
    ), f"fetch after eviction took {elapsed:.0f}ns (writeback+fetch, ≤22 cycles)"
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
    assert (
        cache.mem[24] == 0xDEADBE3F and cache.mem[27] == 0xDEADB33F
    ), "delayed writes not flushed correctly"


@cocotb.test()
async def write_with_stall_while_pending_fetch(dut):
    cache = _setup_wb(dut)
    await cache.reset()

    await cache.write(1, 0x00DEAD00)
    await cache.write(6, 0xCAFEBABE)
    await cache.write(64, 0xDEADB00B)
    await cache.write(68, 0xB00BDEAD)

    assert await cache.read(64) == 0xDEADB00B, "line 64 wrong"
    assert await cache.read(68) == 0xB00BDEAD, "line 68 wrong"

    await cache.flush_line(64)
    await cache.flush_line(68)

    assert (
        cache.mem[64] == 0xDEADB00B and cache.mem[68] == 0xB00BDEAD
    ), "writeback incorrect"


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
    await cache.write(45, 0xEADBBBEE)
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
# NarrowTag (18-bit tag) with 8 words/line -- sl_cluster.vhd's L1 geometry
# ---------------------------------------------------------------------------


def _setup_nt(dut, per_beat_stall_cycles: int = 0):
    NarrowTagCache.start_clock(dut)
    cache = NarrowTagCache(dut, per_beat_stall_cycles=per_beat_stall_cycles)
    cache.start()
    return cache


NT_WORDS = NarrowTagCache.WORDS_PER_LINE
NT_BASE = 2 * NT_WORDS


async def _check_narrow_tag_line_roundtrip(cache):
    """Fill two lines that share a tag-index but differ in tag, let each evict
    the other, and check every word of both landed at its own address -- then
    read one back, which refetches it through the narrow tag compare.

    NarrowTag changes TagWidth and the TagAddrHi/TagAddrLo/TagLow slices, so a
    wrong slice either misses on a line that is present or sends the write-back
    to the wrong address. Both only show up with two lines aliasing each other.
    """
    a = NT_BASE
    b = NT_BASE + NarrowTagCache.LINE_ALIAS

    for i in range(NT_WORDS):
        await cache.write(a + i, 0xA1A1_0000 + i)
    for i in range(NT_WORDS):
        await cache.write(b + i, 0xB2B2_0000 + i)   # evicts a's line
    await cache.flush_line(b)                        # evicts b's line

    for i in range(NT_WORDS):
        assert cache.mem[a + i] == 0xA1A1_0000 + i, (
            f"word {i} of line {a} wrong in memory: "
            f"{cache.mem[a + i]:#010x} != {0xA1A1_0000 + i:#010x}")
        assert cache.mem[b + i] == 0xB2B2_0000 + i, (
            f"word {i} of line {b} wrong in memory: "
            f"{cache.mem[b + i]:#010x} != {0xB2B2_0000 + i:#010x}")

    for i in range(NT_WORDS):
        got = await cache.read(a + i)
        assert got == 0xA1A1_0000 + i, (
            f"word {i} of line {a} wrong after refetch: "
            f"{got:#010x} != {0xA1A1_0000 + i:#010x}")


@cocotb.test(timeout_time=50, timeout_unit="us")
async def narrow_tag_line_roundtrip(dut):
    cache = _setup_nt(dut)
    await cache.reset()
    await _check_narrow_tag_line_roundtrip(cache)


@cocotb.test(timeout_time=50, timeout_unit="us")
async def narrow_tag_line_roundtrip_while_slave_stalls(dut):
    cache = _setup_nt(dut, per_beat_stall_cycles=8)
    await cache.reset()
    await _check_narrow_tag_line_roundtrip(cache)


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

    assert (
        cache.mem[12] == 0xA123_456B and cache.mem[13] == 0xA123_456C
    ), "write-through to same line failed"
    assert await cache.read(12) == 0xA123_456B
    assert await cache.read(13) == 0xA123_456C


@cocotb.test()
async def write_through_to_different_lines(dut):
    cache = _setup_wt(dut)
    await cache.reset()

    await cache.write(14, 0xA123_456D)
    await cache.write(16, 0xA123_456E)
    await Timer(20, unit="ns")

    assert (
        cache.mem[14] == 0xA123_456D and cache.mem[16] == 0xA123_456E
    ), "write-through to different lines failed"
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

    assert (
        await cache.read(33) == 0xDCBA_9913
    ), "cache not reloaded after idle invalidate"


@cocotb.test()
async def invalidate_while_fetching(dut):
    cache = _setup_wt(dut)
    await cache.reset()

    await cache.read(36)
    await cache.backdoor_write(36, 0xDC33_9913)
    await cache.invalidate(36)
    await Timer(40, unit="ns")

    assert (
        await cache.read(36) == 0xDC33_9913
    ), "cache not reloaded after invalidate during fetch"


@cocotb.test()
async def invalidate_while_reading(dut):
    cache = _setup_wt(dut)
    await cache.reset()

    await cache.read(40)
    await cache.backdoor_write(40, 0xDCBB_BB00)

    cache._inv_addr.value = 40
    cache._inv_en.value = 1
    await cache.read(41)
    cache._inv_en.value = 0
    await Timer(40, unit="ns")

    assert (
        await cache.read(40) == 0xDCBB_BB00
    ), "cache not reloaded after invalidate during read"


# ---------------------------------------------------------------------------
# Bypass tests (write-through + EnableBypass, mirrors sl_cluster.vhd's data
# cache; BypassCache.BYPASS_BASE_ADDR = 48)
# ---------------------------------------------------------------------------


def _setup_byp(dut):
    BypassCache.start_clock(dut)
    cache = BypassCache(dut)
    cache.start()
    return cache


@cocotb.test()
async def bypass_read_forwards_directly(dut):
    cache = _setup_byp(dut)
    await cache.reset()

    addr = 100
    cache.mem[addr] = 0x1234_5678

    t0 = cache.sim_time_ns()
    result = await cache.read(addr)
    elapsed = cache.sim_time_ns() - t0

    assert result == 0x1234_5678, f"wrong data: {result:#010x}"
    assert (
        elapsed <= 100
    ), f"bypass read took {elapsed:.0f}ns, expected a fast single-word turnaround (not a line fetch)"


@cocotb.test()
async def bypass_write_forwards_directly(dut):
    cache = _setup_byp(dut)
    await cache.reset()

    addr = 120
    await cache.write(addr, 0xABCD_1234)

    assert cache.mem[addr] == 0xABCD_1234, "bypass write did not land in backing memory"
    assert await cache.read(addr) == 0xABCD_1234, "read-back mismatch"


@cocotb.test()
async def bypass_in_cache_indexable_range_still_bypasses(dut):
    cache = _setup_byp(dut)
    await cache.reset()

    # 50 is inside the cache's own 64-word indexable range but >= BYPASS_BASE_ADDR
    addr = 50
    assert addr >= BypassCache.BYPASS_BASE_ADDR

    cache.mem[addr] = 0xAAAA_5555
    assert await cache.read(addr) == 0xAAAA_5555

    # if this address had been (incorrectly) cached, changing backing memory
    # directly and re-reading would return stale data instead of the update
    cache.mem[addr] = 0xBBBB_6666
    result = await cache.read(addr)
    assert (
        result == 0xBBBB_6666
    ), f"got stale {result:#010x} -- address {addr} appears to have been cached despite being >= BypassBaseAddr"


@cocotb.test()
async def bypass_does_not_pollute_cache(dut):
    cache = _setup_byp(dut)
    await cache.reset()

    # addr 10 and addr 72 share the same tag-index slot (bits[5:2] == 2) but
    # 72 is >= BYPASS_BASE_ADDR so it must never touch that slot's tag/data
    cached_addr = 10
    bypass_addr = 72
    assert (cached_addr >> 2) & 0xF == (bypass_addr >> 2) & 0xF
    assert bypass_addr >= BypassCache.BYPASS_BASE_ADDR

    cache.mem[cached_addr] = 0x1111_2222
    assert (
        await cache.read(cached_addr) == 0x1111_2222
    )  # cold fetch, allocates the slot

    cache.mem[bypass_addr] = 0x3333_4444
    assert (
        await cache.read(bypass_addr) == 0x3333_4444
    )  # must not evict/corrupt the slot above

    t0 = cache.sim_time_ns()
    result = await cache.read(cached_addr)
    elapsed = cache.sim_time_ns() - t0

    assert (
        result == 0x1111_2222
    ), f"addr {cached_addr} corrupted by bypass traffic to colliding index: got {result:#010x}"
    assert (
        elapsed <= 20
    ), f"addr {cached_addr} unexpectedly re-fetched ({elapsed:.0f}ns) -- line was evicted by bypass traffic"


@cocotb.test()
async def bypass_blocks_until_background_linefill_completes(dut):
    """A read miss returns complete_o as soon as the critical (requested)
    word arrives, while the rest of the line keeps streaming in the
    background (state stays non-idle). A bypass request issued right after
    must wait for that background fill to finish before it can use the
    cache's shared downstream wb_master."""
    cache = _setup_byp(dut)
    await cache.reset()

    line_addr = 0
    cache.mem[line_addr] = 0xCAFE_F00D
    assert await cache.read(line_addr) == 0xCAFE_F00D  # triggers the line fetch

    bypass_addr = 100
    cache.mem[bypass_addr] = 0x9999_AAAA

    t0 = cache.sim_time_ns()
    result = await cache.read(bypass_addr)
    elapsed = cache.sim_time_ns() - t0

    assert result == 0x9999_AAAA, f"wrong data: {result:#010x}"
    assert elapsed > 100, (
        f"bypass completed in {elapsed:.0f}ns -- expected it to wait for the "
        f"still-in-progress background line-fill (baseline idle bypass is <=100ns)"
    )


# ---------------------------------------------------------------------------
# Stress tests
# ---------------------------------------------------------------------------

SEED = 0xDEAD_BEEF
MEM_SIZE = 256
N_READS = 1024 * 4
N_WRITES = 512 * 4
N_INVS = 433 * 4


async def _stress(cache, n_reads, n_writes, n_invalidates=0):
    rng = random.Random(SEED)
    ref = list(cache.mem[:MEM_SIZE])

    for addr in range(MEM_SIZE):
        ref[addr] = await cache.read(addr)

    r, w, inv = n_reads, n_writes, n_invalidates
    wdata = 0x0000_1111

    while r or w or inv:
        total = r + w + inv
        roll = rng.randrange(total)

        if roll < r:
            addr = rng.randrange(MEM_SIZE)
            result = await cache.read(addr)
            assert (
                result == ref[addr]
            ), f"read[{addr}]: got {result:#010x}, expected {ref[addr]:#010x}"
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
        assert (
            result == ref[addr]
        ), f"final check[{addr}]: got {result:#010x}, expected {ref[addr]:#010x}"


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
