"""UartBus — cocotb-side 8N1 UART byte driver, pin-compatible with uart.vhd."""

from cocotb.triggers import ClockCycles, ReadOnly, RisingEdge


class UartBus:
    def __init__(self, dut, clock_freq_hz: int, baud_rate: int,
                 rxd_name: str = "uart_rxd_i", txd_name: str = "uart_txd_o"):
        self._clk  = dut.clk_i
        self._rxd  = getattr(dut, rxd_name)  # host -> device (we drive)
        self._txd  = getattr(dut, txd_name)  # device -> host (we sample)
        self._cycles_per_bit = clock_freq_hz // baud_rate
        self._rxd.value = 1  # idle

    async def send_byte(self, byte: int):
        self._rxd.value = 0  # start bit
        await ClockCycles(self._clk, self._cycles_per_bit)
        for i in range(8):
            self._rxd.value = (byte >> i) & 1
            await ClockCycles(self._clk, self._cycles_per_bit)
        self._rxd.value = 1  # stop bit
        await ClockCycles(self._clk, self._cycles_per_bit)

    async def recv_byte(self) -> int:
        while int(self._txd.value) == 1:
            await RisingEdge(self._clk)
        await ClockCycles(self._clk, self._cycles_per_bit + self._cycles_per_bit // 2)
        value = 0
        for i in range(8):
            await ReadOnly()
            value |= int(self._txd.value) << i
            await ClockCycles(self._clk, self._cycles_per_bit)
        return value
