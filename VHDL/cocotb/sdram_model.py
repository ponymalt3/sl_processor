"""SdramModel — behavioral SDR SDRAM, pin-compatible with wb_sdram.vhd.

Decodes standard SDR commands off cs_n/ras_n/cas_n/we_n (ACTIVATE, READ,
WRITE, PRECHARGE, AUTO REFRESH, LOAD MODE REGISTER) and answers with the
same x16, burst-of-2-per-32-bit-word, CAS-latency timing wb_sdram.vhd
expects. PRECHARGE/REFRESH are accepted but no-ops -- correctness doesn't
depend on modeling real row/bank conflicts since wb_sdram always fully
closes the row before the next access.

Geometry defaults match wb_sdram.vhd's own generic defaults (RowBits=13,
ColBits=9, BankBits=2); CAS latency is parsed live off LOAD MODE REGISTER
so it tracks whatever the DUT is actually configured with.

Note on dq: this model never touches wb_sdram.vhd's sdram_dq_io inout pin
directly. GHDL's VPI does not resolve a value cocotb writes to a
bidirectional port against that port's own RTL driver the way two real
VHDL drivers would -- once cocotb deposits (or releases) any value there,
it permanently shadows wb_sdram.vhd's own dq_oe/dq_out drive, so every
later read comes back 'Z'/'U' instead of what the DUT is actually driving.
Callers must give this model the three flat, single-driver
sdram_model_din_o/dout_i/oe_i ports instead (see wb_sdram_wrapper.vhd),
which do the tri-state mux as a second real VHDL driver on the internal
bus signal.
"""

import cocotb
from cocotb.triggers import RisingEdge

CMD_ACTIVATE = (0, 1, 1)  # (ras_n, cas_n, we_n)
CMD_READ     = (1, 0, 1)
CMD_WRITE    = (1, 0, 0)
CMD_MODE_REG = (0, 0, 0)


class SdramModel:
    def __init__(self, dut, prefix: str = "sdram_",
                 row_bits: int = 13, col_bits: int = 9, bank_bits: int = 2,
                 cas_latency: int = 3):
        self._clk   = dut.clk_i
        self._cs_n  = getattr(dut, f"{prefix}cs_n_o")
        self._ras_n = getattr(dut, f"{prefix}ras_n_o")
        self._cas_n = getattr(dut, f"{prefix}cas_n_o")
        self._we_n  = getattr(dut, f"{prefix}we_n_o")
        self._ba    = getattr(dut, f"{prefix}ba_o")
        self._addr  = getattr(dut, f"{prefix}addr_o")
        self._dqm   = getattr(dut, f"{prefix}dqm_o")

        # model-facing tri-state bus, see module docstring
        self._dq_in  = getattr(dut, f"{prefix}model_din_o")   # current bus value
        self._dq_out = getattr(dut, f"{prefix}model_dout_i")  # our drive data
        self._dq_oe  = getattr(dut, f"{prefix}model_oe_i")    # our output-enable

        self._col_bits = col_bits
        self._cas_latency = cas_latency

        self.mem = {}  # (bank, row, col) -> 16-bit value

    def _release_dq(self):
        self._dq_oe.value = 0

    def _drive_dq(self, value: int):
        self._dq_out.value = value
        self._dq_oe.value = 1

    def start(self):
        """Start the background responder coroutine."""
        self._release_dq()
        cocotb.start_soon(self._run())

    async def _run(self):
        bank = 0
        row = 0
        cycle = 0
        pending_reads = []  # list of (deliver_at_cycle, value)

        while True:
            await RisingEdge(self._clk)
            cycle += 1

            delivered = False
            still_pending = []
            for deliver_at, value in pending_reads:
                if deliver_at == cycle:
                    self._drive_dq(value)
                    delivered = True
                else:
                    still_pending.append((deliver_at, value))
            pending_reads = still_pending
            if not delivered:
                self._release_dq()

            if int(self._cs_n.value) == 1:
                continue

            cmd = (int(self._ras_n.value), int(self._cas_n.value), int(self._we_n.value))
            addr = int(self._addr.value)

            if cmd == CMD_ACTIVATE:
                bank = int(self._ba.value)
                row = addr
            elif cmd == CMD_MODE_REG:
                self._cas_latency = (addr >> 4) & 0x7
            elif cmd == CMD_READ:
                col = addr & ((1 << self._col_bits) - 1)
                # -1: cocotb's RisingEdge callback runs after wb_sdram.vhd's
                # own clocked process has already evaluated for this same
                # edge, so a value asserted "at" cycle N is only visible to
                # synchronous RTL from cycle N+1 onward. wb_sdram samples
                # dq on the edge where its CAS0_WAIT/CAS1_WAIT countdown
                # reaches 0, i.e. cas_latency cycles after the command --
                # we must be driving the bus one cycle before that.
                pending_reads.append((cycle + self._cas_latency - 1, self.mem.get((bank, row, col), 0)))
            elif cmd == CMD_WRITE:
                col = addr & ((1 << self._col_bits) - 1)
                dqm = int(self._dqm.value)
                data = int(self._dq_in.value)
                old = self.mem.get((bank, row, col), 0)
                new = old
                if not (dqm & 0b01):
                    new = (new & 0xFF00) | (data & 0x00FF)
                if not (dqm & 0b10):
                    new = (new & 0x00FF) | (data & 0xFF00)
                self.mem[(bank, row, col)] = new
            # PRECHARGE / AUTO REFRESH: no-op
