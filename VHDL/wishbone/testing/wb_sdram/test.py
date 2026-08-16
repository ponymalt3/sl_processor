"""wb_sdram cocotb tests.

Two DUTs behind real wb_masters (see wrapper.vhd): a_ with the normal refresh
interval, b_ with a 1us one so refreshes keep cutting into bursts.

Besides plain data round-trips these check the point of the burst
optimisation itself: that a burst staying inside one row costs exactly one
ACTIVATE and runs at the x16 bus limit of ~2 cycles per 32-bit word.
"""

import random

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import RisingEdge, Timer
from cocotb.utils import get_sim_time

from sdram_model import SdramModel
from wb_master import WbMaster

CLK_PERIOD_NS = 20

ROW_BITS, COL_BITS, BANK_BITS = 13, 9, 2

# wb_sdram's address decode: word address -> col_base | bank | row, with each
# 32-bit word occupying two consecutive x16 columns.
WORDS_PER_ROW = 1 << (COL_BITS - 1)          # 256 words per row per bank
BANK_STRIDE   = WORDS_PER_ROW                # next bank
ROW_STRIDE    = WORDS_PER_ROW << BANK_BITS   # next row


def decode(word_addr: int):
    """(bank, row, col_lo, col_hi) the model stores this word under."""
    col_base = word_addr & (WORDS_PER_ROW - 1)
    bank = (word_addr >> (COL_BITS - 1)) & ((1 << BANK_BITS) - 1)
    row = word_addr >> (COL_BITS - 1 + BANK_BITS)
    return bank, row, col_base * 2, col_base * 2 + 1


def model_word(model: SdramModel, word_addr: int) -> int:
    bank, row, lo, hi = decode(word_addr)
    return (model.mem.get((bank, row, hi), 0) << 16) | model.mem.get((bank, row, lo), 0)


class CmdCounter:
    """Counts SDRAM commands, so a test can assert on row handling."""

    def __init__(self, dut, prefix):
        self._clk   = dut.clk_i
        self._cs_n  = getattr(dut, f"{prefix}cs_n_o")
        self._ras_n = getattr(dut, f"{prefix}ras_n_o")
        self._cas_n = getattr(dut, f"{prefix}cas_n_o")
        self._we_n  = getattr(dut, f"{prefix}we_n_o")
        self.activate = 0
        self.precharge = 0
        self.read = 0
        self.write = 0
        self.refresh = 0

    def start(self):
        cocotb.start_soon(self._run())

    def reset(self):
        self.activate = self.precharge = self.read = self.write = self.refresh = 0

    async def _run(self):
        while True:
            await RisingEdge(self._clk)
            if int(self._cs_n.value) == 1:
                continue
            cmd = (int(self._ras_n.value), int(self._cas_n.value), int(self._we_n.value))
            if cmd == (0, 1, 1):
                self.activate += 1
            elif cmd == (0, 1, 0):
                self.precharge += 1
            elif cmd == (1, 0, 1):
                self.read += 1
            elif cmd == (1, 0, 0):
                self.write += 1
            elif cmd == (0, 0, 1):
                self.refresh += 1


async def setup(dut, port: str):
    """Clock, reset, models, masters and command counters for one test.

    Per test, not once for the whole run: cocotb cancels every task a test
    started when that test ends, so clock and models have to be brought up
    again each time (which also gives every test a freshly reset DUT).
    """
    cocotb.start_soon(Clock(dut.clk_i, CLK_PERIOD_NS, unit="ns").start())

    env = {}
    for p in ("a", "b"):
        model = SdramModel(dut, prefix=f"{p}_sdram_")
        model.start()
        master = WbMaster(dut, prefix=f"{p}_")
        master.idle()
        counter = CmdCounter(dut, f"{p}_sdram_")
        counter.start()
        env[p] = (model, master, counter)

    dut.reset_n_i.value = 0
    await Timer(200, unit="ns")
    dut.reset_n_i.value = 1
    # power-up wait (InitDelayUs=1.0) + precharge/refresh/mode register
    await Timer(3, unit="us")

    return env[port]


async def cycles(coro) -> int:
    """Clock cycles a coroutine takes."""
    start = get_sim_time("ns")
    result = await coro
    return result, int((get_sim_time("ns") - start) // CLK_PERIOD_NS)


@cocotb.test()
async def single_word_roundtrip(dut):
    model, master, _ = await setup(dut, "a")

    for addr, data in ((0, 0xDEADBEEF), (1, 0x01234567), (WORDS_PER_ROW + 3, 0xCAFEF00D)):
        await master.write(addr, data)
        assert model_word(model, addr) == data, (
            f"word {addr}: SDRAM holds {model_word(model, addr):#010x}, wrote {data:#010x} -- "
            "address decode or write data path is wrong")
        got = await master.read(addr)
        assert got == data, f"word {addr}: read back {got:#010x}, wrote {data:#010x}"


@cocotb.test()
async def burst_roundtrip(dut):
    model, master, _ = await setup(dut, "a")
    rnd = random.Random(1)

    # same row, next bank, next row, and a line high up in the address space
    for base in (0, 8, BANK_STRIDE, ROW_STRIDE, 5 * ROW_STRIDE + 2 * BANK_STRIDE + 64):
        data = [rnd.getrandbits(32) for _ in range(8)]
        await master.write_burst(base, data)
        for i, word in enumerate(data):
            assert model_word(model, base + i) == word, (
                f"burst at {base}, word {i}: SDRAM holds "
                f"{model_word(model, base + i):#010x}, wrote {word:#010x}")
        got = await master.read_burst(base, 8)
        assert got == data, f"burst at {base}: read back {got}, wrote {data}"


@cocotb.test()
async def burst_keeps_row_open(dut):
    """The optimisation itself: one ACTIVATE per burst, ~2 cycles per word."""
    model, master, counter = await setup(dut, "a")
    rnd = random.Random(2)

    base = 3 * ROW_STRIDE + 16
    data = [rnd.getrandbits(32) for _ in range(8)]

    # a refresh landing inside the measured window closes the row and costs
    # cycles -- that is correct behaviour, just not what this test measures
    for _attempt in range(5):
        counter.reset()
        _, write_cycles = await cycles(master.write_burst(base, data))
        if counter.refresh == 0:
            break
    assert counter.activate == 1, (
        f"8-word write burst issued {counter.activate} ACTIVATEs, expected 1 -- "
        "the row is not being kept open across the burst")
    assert counter.write == 16, f"expected 16 column writes, got {counter.write}"

    # second burst in the same row: no ACTIVATE at all, the row is still open
    for _attempt in range(5):
        counter.reset()
        got, read_cycles = await cycles(master.read_burst(base, 8))
        assert got == data, f"read back {got}, wrote {data}"
        if counter.refresh == 0:
            break
    assert counter.activate == 0, (
        f"read burst into the already open row issued {counter.activate} ACTIVATEs, expected 0")
    assert counter.read == 16, f"expected 16 column reads, got {counter.read}"

    # x16 bus limit is 2 cycles per 32-bit word; the rest is CAS latency and
    # wb_master's own start/end overhead. The old row-per-word controller
    # needed ~15 cycles per word.
    dut._log.info(f"8-word burst: {write_cycles} cycles write, {read_cycles} cycles read")
    assert write_cycles <= 24, f"write burst took {write_cycles} cycles for 8 words"
    assert read_cycles <= 30, f"read burst took {read_cycles} cycles for 8 words"


@cocotb.test()
async def row_change_and_mixed_access(dut):
    """Reads and writes alternating across rows/banks: turnaround + precharge."""
    model, master, _ = await setup(dut, "a")
    rnd = random.Random(3)

    addrs = [0, ROW_STRIDE + 1, BANK_STRIDE + 2, 2 * ROW_STRIDE + 3, 3, BANK_STRIDE + 2]
    expected = {}
    for addr in addrs:
        value = rnd.getrandbits(32)
        await master.write(addr, value)
        expected[addr] = value
        # read straight back from a different row, then this one again
        other = (addr + ROW_STRIDE) & 0x7FFFF
        await master.read(other)
        got = await master.read(addr)
        assert got == value, f"word {addr}: read back {got:#010x}, wrote {value:#010x}"

    for addr, value in expected.items():
        got = await master.read(addr)
        assert got == value, f"word {addr}: read back {got:#010x}, wrote {value:#010x}"


@cocotb.test()
async def bursts_survive_refresh(dut):
    """DUT b refreshes every 1us (50 cycles), i.e. in the middle of bursts."""
    model, master, counter = await setup(dut, "b")
    rnd = random.Random(4)

    counter.reset()
    for i in range(8):
        base = i * ROW_STRIDE + i * 8
        data = [rnd.getrandbits(32) for _ in range(8)]
        await master.write_burst(base, data)
        got = await master.read_burst(base, 8)
        assert got == data, f"burst at {base} across a refresh: read back {got}, wrote {data}"

    assert counter.refresh > 0, "no refresh happened -- the test proves nothing"
    dut._log.info(f"{counter.refresh} refreshes during the bursts")
