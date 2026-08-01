"""WriteBackCache / WriteThroughCache — driver interfaces for wb_cache.vhd."""

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import RisingEdge, Timer
import cocotb.utils

from wb_master import WbMaster
from wb_memory import WbMemoryModel

CLK_PERIOD_NS = 20


class _CacheBase:
    def __init__(self, dut, prefix: str, per_beat_stall_cycles: int = 0):
        self._dut = dut
        self._clk = dut.clk_i

        self._addr     = getattr(dut, f"{prefix}addr_i")
        self._din      = getattr(dut, f"{prefix}din_i")
        self._dout     = getattr(dut, f"{prefix}dout_o")
        self._en       = getattr(dut, f"{prefix}en_i")
        self._we       = getattr(dut, f"{prefix}we_i")
        self._complete = getattr(dut, f"{prefix}complete_o")

        self._mem_model = WbMemoryModel(dut, prefix=prefix,
                                        per_beat_stall_cycles=per_beat_stall_cycles)

    @property
    def mem(self):
        return self._mem_model.mem

    @staticmethod
    def start_clock(dut):
        """Start the shared DUT clock. Call once per test."""
        cocotb.start_soon(Clock(dut.clk_i, CLK_PERIOD_NS, unit="ns").start())

    def start(self):
        """Idle all inputs and launch the background memory responder."""
        self._idle_cache()
        self._mem_model.start()

    async def reset(self):
        """Assert reset, idle all cache inputs, then release."""
        self._dut.reset_n_i.value = 0
        self._idle_cache()
        await Timer(33, unit="ns")
        self._dut.reset_n_i.value = 1
        await Timer(500, unit="ns")

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

    async def flush_line(self, addr: int):
        """Force writeback of the line containing addr by evicting it."""
        await self.read((addr + 64) % 256)
        await Timer(200, unit="ns")

    @staticmethod
    def sim_time_ns() -> float:
        return cocotb.utils.get_sim_time(unit="ns")

    def _idle_cache(self):
        self._addr.value = 0
        self._din.value  = 0
        self._en.value   = 0
        self._we.value   = 0


class WriteBackCache(_CacheBase):
    """Interface to the write-back cache instance (wb_* ports)."""

    def __init__(self, dut, per_beat_stall_cycles: int = 0):
        super().__init__(dut, prefix="wb_",
                         per_beat_stall_cycles=per_beat_stall_cycles)


class WriteThroughCache(_CacheBase):
    """Interface to the write-through cache instance (wt_* ports)."""

    def __init__(self, dut):
        super().__init__(dut, prefix="wt_")
        self._inv_addr = dut.wt_inv_addr_i
        self._inv_en   = dut.wt_inv_en_i

    def _idle_cache(self):
        super()._idle_cache()
        self._inv_addr.value = 0
        self._inv_en.value   = 0

    async def invalidate(self, addr: int):
        """Assert snooping invalidation and wait for it to propagate through
        the tag DPRAM (needs two extra clock cycles after the pulse)."""
        await RisingEdge(self._clk)
        self._inv_addr.value = addr
        self._inv_en.value   = 1
        await RisingEdge(self._clk)
        self._inv_en.value   = 0
        await RisingEdge(self._clk)
        await RisingEdge(self._clk)

    async def backdoor_write(self, addr: int, data: int):
        """Write directly into the backing memory, bypassing the cache."""
        self.mem[addr] = data


class BypassCache(_CacheBase):
    """Interface to the write-through + EnableBypass cache instance (byp_*
    ports) -- mirrors sl_cluster.vhd's data-cache instantiation. Addresses
    >= BYPASS_BASE_ADDR forward straight through to the backing memory
    without allocating a cache line."""

    BYPASS_BASE_ADDR = 48

    def __init__(self, dut):
        super().__init__(dut, prefix="byp_")


class NarrowTagCache(_CacheBase):
    """Write-back cache with NarrowTag and 8 words/line (nt_* ports) -- the tag
    geometry sl_cluster.vhd's L1s run with. NarrowTag changes TagWidth and the
    TagAddrHi/TagAddrLo/TagLow slices, i.e. the tag compare, the tag update and
    the address the write-back is issued to."""

    WORDS_PER_LINE = 8
    NUM_LINES = 16
    LINE_ALIAS = WORDS_PER_LINE * NUM_LINES  # same index, different tag

    def __init__(self, dut, per_beat_stall_cycles: int = 0):
        super().__init__(dut, prefix="nt_",
                         per_beat_stall_cycles=per_beat_stall_cycles)

    async def flush_line(self, addr: int):
        await self.read(addr + self.LINE_ALIAS)
        await Timer(400, unit="ns")


class CacheThroughArbiter:
    """Write-back cache instance (ix_* ports) reached through a real
    single-master wb_ixs plus wb_cache_adapter(IsConnectedToIXS=true), the
    same topology as sl_system.vhd's debug_ctrl -> wb_ixs_1 -> sdram_cache_1.
    The flat wb_*/wt_*/byp_* ports bypass the arbiter and adapter entirely,
    so only this path exercises their effect on the cache's request timing.
    """

    def __init__(self, dut, per_beat_stall_cycles: int = 0):
        self._dut = dut
        self._clk = dut.clk_i
        self._master = WbMaster(dut, prefix="ix_")
        self._mem_model = WbMemoryModel(dut, prefix="ix_",
                                        per_beat_stall_cycles=per_beat_stall_cycles)

    @property
    def mem(self):
        return self._mem_model.mem

    @staticmethod
    def start_clock(dut):
        cocotb.start_soon(Clock(dut.clk_i, CLK_PERIOD_NS, unit="ns").start())

    def start(self):
        self._master.idle()
        self._mem_model.start()

    async def reset(self):
        self._dut.reset_n_i.value = 0
        self._master.idle()
        await Timer(33, unit="ns")
        self._dut.reset_n_i.value = 1
        await Timer(500, unit="ns")

    async def write(self, addr: int, data: int):
        await self._master.write(addr, data)

    async def read(self, addr: int) -> int:
        return await self._master.read(addr)

    async def flush_line(self, addr: int):
        await self.read((addr + 64) % 256)
        await Timer(200, unit="ns")

    @staticmethod
    def sim_time_ns() -> float:
        return cocotb.utils.get_sim_time(unit="ns")
