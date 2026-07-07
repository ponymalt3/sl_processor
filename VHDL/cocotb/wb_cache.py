"""WriteBackCache / WriteThroughCache — driver interfaces for wb_cache.vhd."""

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import RisingEdge, Timer
import cocotb.utils

from wb_memory import WbMemoryModel

CLK_PERIOD_NS = 20


class _CacheBase:
    def __init__(self, dut, prefix: str, write_delay: int):
        self._dut = dut
        self._clk = dut.clk_i

        self._addr     = getattr(dut, f"{prefix}addr_i")
        self._din      = getattr(dut, f"{prefix}din_i")
        self._dout     = getattr(dut, f"{prefix}dout_o")
        self._en       = getattr(dut, f"{prefix}en_i")
        self._we       = getattr(dut, f"{prefix}we_i")
        self._complete = getattr(dut, f"{prefix}complete_o")

        self._mem_model = WbMemoryModel(dut, prefix=prefix,
                                        write_delay=write_delay)

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

    def __init__(self, dut):
        super().__init__(dut, prefix="wb_", write_delay=1)


class WriteThroughCache(_CacheBase):
    """Interface to the write-through cache instance (wt_* ports)."""

    def __init__(self, dut):
        super().__init__(dut, prefix="wt_", write_delay=0)
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
