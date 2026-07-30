"""DebugCtrl — cocotb driver for the UART debug protocol (wb_debug_ctrl.vhd).

Wire-compatible with the real host tool (Debugger/SystemControl.h):
CMD_READ=0x81, CMD_WRITE=0x82, CMD_CORE=0x03, CMD_PING=0x04. addr is
32-bit, len is 16-bit, and all multi-byte fields go out MSB-first,
matching wb_debug_ctrl.vhd's shift-register byte order.
"""

from uart import UartBus

CMD_READ  = 0x81
CMD_WRITE = 0x82
CMD_CORE  = 0x03
CMD_PING  = 0x04


class DebugCtrl:
    def __init__(self, dut, clock_freq_hz: int, baud_rate: int,
                 rxd_name: str = "uart_rxd_i", txd_name: str = "uart_txd_o"):
        self._uart = UartBus(dut, clock_freq_hz, baud_rate, rxd_name, txd_name)

    async def ping(self) -> bool:
        await self._uart.send_byte(CMD_PING)
        return await self._uart.recv_byte() == CMD_PING

    async def set_cores(self, enable_mask: int, reset_mask: int = 0) -> bool:
        """enable_mask/reset_mask: bit i = core i. A set reset bit holds that core in reset."""
        await self._uart.send_byte(CMD_CORE)
        await self._uart.send_byte(reset_mask & 0xFF)
        await self._uart.send_byte(enable_mask & 0xFF)
        return await self._uart.recv_byte() == CMD_CORE

    async def write(self, addr: int, words) -> bool:
        words = list(words)
        await self._uart.send_byte(CMD_WRITE)
        await self._uart.send_byte((addr >> 24) & 0xFF)
        await self._uart.send_byte((addr >> 16) & 0xFF)
        await self._uart.send_byte((addr >> 8) & 0xFF)
        await self._uart.send_byte(addr & 0xFF)
        await self._uart.send_byte((len(words) >> 8) & 0xFF)
        await self._uart.send_byte(len(words) & 0xFF)
        for w in words:
            await self._uart.send_byte((w >> 24) & 0xFF)
            await self._uart.send_byte((w >> 16) & 0xFF)
            await self._uart.send_byte((w >> 8) & 0xFF)
            await self._uart.send_byte(w & 0xFF)
        return await self._uart.recv_byte() == CMD_WRITE

    async def read(self, addr: int, count: int) -> list:
        await self._uart.send_byte(CMD_READ)
        await self._uart.send_byte((addr >> 24) & 0xFF)
        await self._uart.send_byte((addr >> 16) & 0xFF)
        await self._uart.send_byte((addr >> 8) & 0xFF)
        await self._uart.send_byte(addr & 0xFF)
        await self._uart.send_byte((count >> 8) & 0xFF)
        await self._uart.send_byte(count & 0xFF)
        words = []
        for _ in range(count):
            b3 = await self._uart.recv_byte()
            b2 = await self._uart.recv_byte()
            b1 = await self._uart.recv_byte()
            b0 = await self._uart.recv_byte()
            words.append((b3 << 24) | (b2 << 16) | (b1 << 8) | b0)
        ack = await self._uart.recv_byte()
        assert ack == CMD_READ, f"expected READ ack 0x{CMD_READ:02x}, got 0x{ack:02x}"
        return words
