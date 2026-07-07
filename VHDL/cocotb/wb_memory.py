"""WbMemoryModel — Python-side Wishbone B4 pipelined slave."""

import cocotb
from cocotb.triggers import RisingEdge


class WbMemoryModel:
    """Responds to pipelined Wishbone B4 transactions as a backing memory.

    Parameters
    ----------
    dut        : cocotb DUT handle
    prefix     : signal name prefix (e.g. "wb_" → wb_m_dat_i, wb_m_ack_i …)
    size       : number of 32-bit words
    init_value : initial content of every word (default 0xFFFFFFFF = erased flash)
    write_delay: 0 = write data visible same cycle (write-through / plain RAM)
                 1 = write data registered one cycle later (DPRAM write-first lag,
                     matches the wb_cache write-back path)
    """

    def __init__(self, dut, prefix: str = "", size: int = 512,
                 init_value: int = 0xFFFF_FFFF, write_delay: int = 0):
        self._clk       = dut.clk_i
        self._dat_i     = getattr(dut, f"{prefix}m_dat_i")
        self._ack_i     = getattr(dut, f"{prefix}m_ack_i")
        self._err_i     = getattr(dut, f"{prefix}m_err_i")
        self._stall_i   = getattr(dut, f"{prefix}m_stall_i")
        self._adr_o     = getattr(dut, f"{prefix}m_adr_o")
        self._dat_o     = getattr(dut, f"{prefix}m_dat_o")
        self._we_o      = getattr(dut, f"{prefix}m_we_o")
        self._stb_o     = getattr(dut, f"{prefix}m_stb_o")
        self._cyc_o     = getattr(dut, f"{prefix}m_cyc_o")
        self._write_delay = write_delay
        self.mem = [init_value] * size

    def idle(self):
        self._dat_i.value   = 0
        self._ack_i.value   = 0
        self._err_i.value   = 0
        self._stall_i.value = 0

    def start(self):
        """Start the background responder coroutine."""
        self.idle()
        cocotb.start_soon(self._run())

    async def _run(self):
        self.idle()
        pending_wr   = False
        pending_addr = 0
        while True:
            await RisingEdge(self._clk)

            if self._write_delay and pending_wr:
                self.mem[pending_addr] = int(self._dat_o.value)
                pending_wr = False

            if self._cyc_o.value == 1 and self._stb_o.value == 1:
                addr = int(self._adr_o.value)
                if self._we_o.value == 1:
                    if self._write_delay:
                        pending_wr   = True
                        pending_addr = addr
                    else:
                        self.mem[addr] = int(self._dat_o.value)
                else:
                    self._dat_i.value = self.mem[addr]
                self._ack_i.value = 1
            else:
                self._ack_i.value = 0
