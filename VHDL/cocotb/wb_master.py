"""WbMaster — drives wb_master.vhd's high-level interface ports."""

from cocotb.triggers import RisingEdge


class WbMaster:
    """Drives one wb_master's high-level interface.

    en acts as a one-cycle pulse: de-asserted immediately after the first
    rising edge so the master cannot re-enter PENDING spuriously when it
    returns to IDLE after completing a transaction.
    """

    def __init__(self, dut, prefix: str):
        self._clk      = dut.clk_i
        self._addr     = getattr(dut, f"{prefix}addr_i")
        self._din      = getattr(dut, f"{prefix}din_i")
        self._dout     = getattr(dut, f"{prefix}dout_o")
        self._en       = getattr(dut, f"{prefix}en_i")
        self._burst    = getattr(dut, f"{prefix}burst_i")
        self._we       = getattr(dut, f"{prefix}we_i")
        self._dready   = getattr(dut, f"{prefix}dready_o")
        self._complete = getattr(dut, f"{prefix}complete_o")

    def idle(self):
        self._addr.value  = 0
        self._din.value   = 0
        self._en.value    = 0
        self._burst.value = 0
        self._we.value    = 0

    async def write(self, addr: int, data: int):
        self._addr.value  = addr
        self._din.value   = data
        self._en.value    = 1
        self._we.value    = 1
        self._burst.value = 0
        await RisingEdge(self._clk)
        self._en.value = 0
        self._we.value = 0
        while not self._complete.value:
            await RisingEdge(self._clk)
        await RisingEdge(self._clk)

    async def read(self, addr: int) -> int:
        self._addr.value  = addr
        self._din.value   = 0
        self._en.value    = 1
        self._we.value    = 0
        self._burst.value = 0
        await RisingEdge(self._clk)
        self._en.value = 0
        while not self._complete.value:
            await RisingEdge(self._clk)
        result = int(self._dout.value)
        await RisingEdge(self._clk)
        return result

    async def write_burst(self, addr: int, data: list):
        n = len(data)
        self._addr.value  = addr
        self._din.value   = data[0]
        self._en.value    = 1
        self._we.value    = 1
        self._burst.value = n - 1
        idx = 0
        await RisingEdge(self._clk)
        self._en.value = 0
        while not self._complete.value:
            await RisingEdge(self._clk)
            if self._dready.value == 1 and idx + 1 < n:
                idx += 1
                self._din.value = data[idx]
        self._we.value = 0
        await RisingEdge(self._clk)

    async def read_burst(self, addr: int, count: int) -> list:
        self._addr.value  = addr
        self._din.value   = 0
        self._en.value    = 1
        self._we.value    = 0
        self._burst.value = count - 1
        result = []
        await RisingEdge(self._clk)
        self._en.value = 0
        while not self._complete.value:
            await RisingEdge(self._clk)
            if self._dready.value == 1:
                result.append(int(self._dout.value))
        if len(result) < count:
            result.append(int(self._dout.value))
        await RisingEdge(self._clk)
        return result
