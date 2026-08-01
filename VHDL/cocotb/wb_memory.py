"""WbMemoryModel — Python-side Wishbone B4 pipelined slave."""

import cocotb
from cocotb.triggers import RisingEdge


class WbMemoryModel:
    """Responds to pipelined Wishbone B4 transactions as a backing memory.

    A write stores the data presented alongside its address in the same cycle,
    which is what a real slave does: wb_mem.vhd registers adr/dat/we together
    and writes them as one.

    Parameters
    ----------
    dut        : cocotb DUT handle
    prefix     : signal name prefix (e.g. "wb_" → wb_m_dat_i, wb_m_ack_i …)
    size       : number of 32-bit words
    init_value : initial content of every word (default 0xFFFFFFFF = erased flash)
    per_beat_stall_cycles: stall cycles inserted before EVERY beat's ack,
                 including beats inside a continuously-selected burst. Models a
                 controller that closes the SDRAM row between words -- wb_sdram.vhd
                 holds stall high for almost the whole per-word transaction
                 (`stall <= '0' when state = ST_IDLE else '1'`). 0 = ack as fast
                 as possible.
    """

    def __init__(self, dut, prefix: str = "", size: int = 512,
                 init_value: int = 0xFFFF_FFFF, per_beat_stall_cycles: int = 0):
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
        self._per_beat_stall_cycles = per_beat_stall_cycles
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
        busy       = False
        busy_addr  = 0
        busy_we    = False
        busy_dat   = 0
        stall_left = 0
        while True:
            await RisingEdge(self._clk)

            self._ack_i.value = 0

            if busy:
                # Finishing a beat that was already accepted (cyc=1, stb=1,
                # stall=0 seen) on an earlier cycle -- deliberately does NOT
                # re-check live stb/cyc here. A pipelined master may drop stb
                # as soon as it has presented every beat's address, well before
                # every ack has come back; wb_master.vhd does exactly that.
                # Gating on live stb would abandon outstanding beats and hang
                # the master waiting for an ack that never arrives.
                if stall_left > 0:
                    stall_left -= 1
                    continue
                if busy_we:
                    self.mem[busy_addr] = busy_dat
                else:
                    self._dat_i.value = self.mem[busy_addr]
                self._ack_i.value   = 1
                self._stall_i.value = 0
                busy = False
                continue

            if not (self._cyc_o.value == 1 and self._stb_o.value == 1):
                self._stall_i.value = 0
                continue

            if self._per_beat_stall_cycles > 0:
                stall_left = self._per_beat_stall_cycles
                busy       = True
                busy_addr  = int(self._adr_o.value)
                busy_we    = self._we_o.value == 1
                busy_dat   = int(self._dat_o.value) if busy_we else 0
                self._stall_i.value = 1
                continue

            self._stall_i.value = 0
            addr = int(self._adr_o.value)
            if self._we_o.value == 1:
                self.mem[addr] = int(self._dat_o.value)
            else:
                self._dat_i.value = self.mem[addr]
            self._ack_i.value = 1
