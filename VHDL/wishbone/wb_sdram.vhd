library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

use work.wishbone_p.all;

-- Wishbone-native SDR SDRAM controller, defaults matched to the DE0-Nano's
-- 32MB x16 4-bank SDRAM at 50MHz (13-bit row, 9-bit column).
--
-- Optimised for the cache line traffic of wb_cache/wb_master: the row opened
-- by an access stays open, and column commands are issued back to back, one
-- per cycle. A 32-bit Wishbone word is two x16 column accesses, so a burst
-- that stays inside the open row runs at the bus maximum of 2 cycles/word --
-- the ACTIVATE/tRCD/PRECHARGE overhead is paid once per row change instead
-- of once per word. Row misses and refreshes close the row again.
--
-- The Wishbone side is pipelined: requests are accepted while earlier reads
-- are still in flight, acks come back in order (read acks out of the CAS
-- latency pipeline, write acks when the second column command is issued).
-- Burst mode of the SDRAM itself stays at burst length 1 -- every column is
-- commanded explicitly, which keeps wrapping bursts and byte masking simple.
entity wb_sdram is
  generic (
    ClockFreqHz       : natural := 50_000_000;
    RowBits           : natural := 13;
    ColBits           : natural := 9;
    BankBits          : natural := 2;
    CasLatency        : natural range 2 to 3 := 3;
    InitDelayUs       : real := 100.0;
    T_RCD_Ns          : real := 20.0;
    T_RP_Ns           : real := 20.0;
    T_RAS_Ns          : real := 42.0;
    T_RFC_Ns          : real := 70.0;
    T_WR_Ns           : real := 14.0;
    T_MRD_Cycles      : natural := 3;
    RefreshIntervalUs : real := 15.625;
    InitRefreshCount  : natural := 2);

  port (
    clk_i     : in std_ulogic;
    reset_n_i : in std_ulogic;

    slave_i : in  wb_slave_ifc_in_t;
    slave_o : out wb_slave_ifc_out_t;

    sdram_clk_o   : out std_ulogic;
    sdram_cke_o   : out std_ulogic;
    sdram_cs_n_o  : out std_ulogic;
    sdram_ras_n_o : out std_ulogic;
    sdram_cas_n_o : out std_ulogic;
    sdram_we_n_o  : out std_ulogic;
    sdram_ba_o    : out std_ulogic_vector(BankBits-1 downto 0);
    sdram_addr_o  : out std_ulogic_vector(RowBits-1 downto 0);
    sdram_dqm_o   : out std_ulogic_vector(1 downto 0);
    sdram_dq_io   : inout std_logic_vector(15 downto 0));

end entity wb_sdram;

architecture rtl of wb_sdram is

  constant ClockPeriodNs : real := 1.0e9/real(ClockFreqHz);

  function sub_clip (a, b : natural) return natural is
  begin
    if a > b then
      return a-b;
    end if;
    return 0;
  end function;

  function ns_to_cycles (t_ns : real) return natural is
  begin
    return natural(ceil(t_ns/ClockPeriodNs));
  end function;

  constant InitCycles    : natural := natural(ceil(InitDelayUs*1000.0/ClockPeriodNs));
  constant RcdCycles     : natural := ns_to_cycles(T_RCD_Ns);
  constant RpCycles      : natural := ns_to_cycles(T_RP_Ns);
  constant RasCycles     : natural := ns_to_cycles(T_RAS_Ns);
  constant RfcCycles     : natural := ns_to_cycles(T_RFC_Ns);
  constant WrCycles      : natural := ns_to_cycles(T_WR_Ns);
  constant RefreshCycles : natural := natural(RefreshIntervalUs*1000.0/ClockPeriodNs);

  -- wait state counts: a command in cycle T followed by <X>Wait+1 idle cycles
  -- puts the next command in cycle T+<X>Cycles. Only used where the timing
  -- needs more than one cycle -- otherwise the wait state is skipped entirely.
  constant RcdWait : natural := sub_clip(RcdCycles,2);
  constant RpWait  : natural := sub_clip(RpCycles,2);
  constant RfcWait : natural := sub_clip(RfcCycles,2);

  constant MaxWait : natural := InitCycles-1;

  constant CasLatencyCode : std_ulogic_vector(2 downto 0) := std_ulogic_vector(to_unsigned(CasLatency,3));
  -- burst length 1, sequential, standard operation
  constant ModeRegVal : std_ulogic_vector(12 downto 0) := "000" & "0" & "00" & CasLatencyCode & "0" & "000";

  type state_t is (
    ST_INIT_WAIT, ST_INIT_PRECHARGE, ST_INIT_PRECHARGE_WAIT,
    ST_INIT_REFRESH, ST_INIT_REFRESH_WAIT, ST_INIT_MODE, ST_INIT_MODE_WAIT,
    ST_IDLE, ST_DRAIN,
    ST_PRECHARGE, ST_PRECHARGE_WAIT,
    ST_ACTIVATE, ST_ACTIVATE_WAIT,
    ST_CAS0, ST_CAS1,
    ST_REFRESH, ST_REFRESH_WAIT);

  signal state : state_t;
  signal wait_count : natural range 0 to MaxWait;
  signal init_refresh_left : natural range 0 to InitRefreshCount-1;
  signal refresh_timer : natural range 0 to RefreshCycles-1;
  signal refresh_due : std_ulogic;
  signal refresh_pending : std_ulogic;

  -- request currently being issued (latched on accept)
  signal req_we  : std_ulogic;
  signal req_sel : std_ulogic_vector(3 downto 0);
  signal req_dat : std_ulogic_vector(31 downto 0);
  signal req_bank : unsigned(BankBits-1 downto 0);
  signal req_row  : unsigned(RowBits-1 downto 0);
  signal req_col  : unsigned(ColBits-2 downto 0);
  signal req_row_hit : std_ulogic;

  -- currently open row
  signal row_open  : std_ulogic;
  signal open_bank : unsigned(BankBits-1 downto 0);
  signal open_row  : unsigned(RowBits-1 downto 0);

  signal ras_timer : natural range 0 to RasCycles;
  signal wr_timer  : natural range 0 to WrCycles;

  -- read tracking: a '1' shifted in for every column read command, tagged in
  -- rd_half with which half of the 32-bit word it fetches. The marker sits at
  -- index CasLatency-1 in the cycle the SDRAM drives that column's data.
  signal rd_pipe : std_ulogic_vector(CasLatency downto 0);
  signal rd_half : std_ulogic_vector(CasLatency downto 0);
  signal rd_idle : std_ulogic;

  signal read_low  : std_ulogic_vector(15 downto 0);
  signal read_data : std_ulogic_vector(31 downto 0);
  signal ack : std_ulogic;

  signal dq_out : std_ulogic_vector(15 downto 0);
  signal dq_oe  : std_ulogic;
  signal stall  : std_ulogic;

  signal adr_bank : unsigned(BankBits-1 downto 0);
  signal adr_row  : unsigned(RowBits-1 downto 0);
  signal row_hit  : std_ulogic;
  signal ready    : std_ulogic;
  signal accept   : std_ulogic;

begin

  assert ColBits <= 10
    report "wb_sdram: ColBits > 10 collides with the A10 auto-precharge bit"
    severity failure;

  sdram_clk_o <= clk_i;
  sdram_cke_o <= '1';
  sdram_dq_io <= To_StdLogicVector(dq_out) when dq_oe = '1' else (others => 'Z');

  -- address decode of the incoming (not yet accepted) request
  adr_bank <= slave_i.adr(ColBits+BankBits-2 downto ColBits-1);
  adr_row  <= slave_i.adr(ColBits+BankBits+RowBits-2 downto ColBits+BankBits-1);
  row_hit  <= '1' when row_open = '1' and adr_bank = open_bank and adr_row = open_row else '0';

  rd_idle <= '1' when rd_pipe = (rd_pipe'range => '0') else '0';

  -- ST_CAS1 is ready too: the running access has issued both its column
  -- commands, so the next word can be accepted without a gap. Depends only on
  -- registered state, never on the incoming request -- no combinational path
  -- from the master's request into stall.
  ready <= '1' when (state = ST_IDLE or state = ST_CAS1) and
                    refresh_due = '0' and refresh_pending = '0' else '0';
  accept <= ready and slave_i.cyc and slave_i.stb;

  stall <= not ready;
  slave_o <= (read_data,ack,'0',stall);

  -- command / datapath output decode (Moore: depends only on current state
  -- and the request latched on accept)
  process (state, req_we, req_sel, req_dat, req_bank, req_row, req_col) is
  begin
    sdram_cs_n_o  <= '0';
    sdram_ras_n_o <= '1';
    sdram_cas_n_o <= '1';
    sdram_we_n_o  <= '1';
    sdram_ba_o    <= (others => '0');
    sdram_addr_o  <= (others => '0');
    sdram_dqm_o   <= "00";
    dq_oe  <= '0';
    dq_out <= (others => '0');

    case state is
      when ST_INIT_PRECHARGE | ST_PRECHARGE =>
        sdram_ras_n_o <= '0';
        sdram_we_n_o  <= '0';
        sdram_addr_o(10) <= '1';         -- precharge all banks

      when ST_INIT_REFRESH | ST_REFRESH =>
        sdram_ras_n_o <= '0';
        sdram_cas_n_o <= '0';            -- auto refresh

      when ST_INIT_MODE =>
        sdram_ras_n_o <= '0';
        sdram_cas_n_o <= '0';
        sdram_we_n_o  <= '0';            -- load mode register
        sdram_addr_o(ModeRegVal'range) <= ModeRegVal;

      when ST_ACTIVATE =>
        sdram_ras_n_o <= '0';
        sdram_ba_o <= std_ulogic_vector(req_bank);
        sdram_addr_o(RowBits-1 downto 0) <= std_ulogic_vector(req_row);

      when ST_CAS0 | ST_CAS1 =>
        sdram_cas_n_o <= '0';
        sdram_we_n_o  <= not req_we;
        sdram_ba_o <= std_ulogic_vector(req_bank);
        sdram_addr_o(ColBits-1 downto 1) <= std_ulogic_vector(req_col);
        sdram_addr_o(10) <= '0';         -- no auto-precharge, row stays open
        if state = ST_CAS0 then
          sdram_addr_o(0) <= '0';
        else
          sdram_addr_o(0) <= '1';
        end if;
        if req_we = '1' then
          dq_oe <= '1';
          if state = ST_CAS0 then
            dq_out <= req_dat(15 downto 0);
            sdram_dqm_o <= not req_sel(1 downto 0);
          else
            dq_out <= req_dat(31 downto 16);
            sdram_dqm_o <= not req_sel(3 downto 2);
          end if;
        end if;

      when others => null;             -- NOP
    end case;
  end process;

  process (clk_i, reset_n_i) is
    -- next state once the current request may finally be issued
    procedure start_request is
    begin
      if req_row_hit = '1' then
        state <= ST_CAS0;
      elsif row_open = '1' then
        state <= ST_PRECHARGE;
      else
        state <= ST_ACTIVATE;
      end if;
    end procedure;

    -- next state after a PRECHARGE has completed
    procedure after_precharge is
    begin
      if refresh_pending = '1' then
        state <= ST_REFRESH;
      else
        state <= ST_ACTIVATE;
      end if;
    end procedure;
  begin
    if reset_n_i = '0' then
      state <= ST_INIT_WAIT;
      wait_count <= InitCycles-1;
      init_refresh_left <= InitRefreshCount-1;
      refresh_timer <= 0;
      refresh_due <= '0';
      refresh_pending <= '0';
      req_we <= '0';
      req_sel <= (others => '0');
      req_dat <= (others => '0');
      req_bank <= (others => '0');
      req_row <= (others => '0');
      req_col <= (others => '0');
      req_row_hit <= '0';
      row_open <= '0';
      open_bank <= (others => '0');
      open_row <= (others => '0');
      ras_timer <= 0;
      wr_timer <= 0;
      rd_pipe <= (others => '0');
      rd_half <= (others => '0');
      read_low <= (others => '0');
      read_data <= (others => '0');
      ack <= '0';
    elsif clk_i'event and clk_i = '1' then
      ack <= '0';

      if refresh_timer = RefreshCycles-1 then
        refresh_timer <= 0;
        refresh_due <= '1';
      else
        refresh_timer <= refresh_timer+1;
      end if;

      if ras_timer /= 0 then
        ras_timer <= ras_timer-1;
      end if;
      if wr_timer /= 0 then
        wr_timer <= wr_timer-1;
      end if;

      -- read pipeline: shift in a marker for the column read command being
      -- driven in this cycle, capture the data CasLatency cycles later
      rd_pipe <= rd_pipe(CasLatency-1 downto 0) & '0';
      rd_half <= rd_half(CasLatency-1 downto 0) & '0';
      if (state = ST_CAS0 or state = ST_CAS1) and req_we = '0' then
        rd_pipe(0) <= '1';
        if state = ST_CAS1 then
          rd_half(0) <= '1';
        end if;
      end if;

      if rd_pipe(CasLatency-1) = '1' then
        if rd_half(CasLatency-1) = '1' then
          read_data <= To_StdULogicVector(sdram_dq_io) & read_low;
          ack <= '1';
        else
          read_low <= To_StdULogicVector(sdram_dq_io);
        end if;
      end if;

      -- request accept, shared by every ready state
      if accept = '1' then
        req_we  <= slave_i.we;
        req_sel <= slave_i.sel;
        req_dat <= slave_i.dat;
        req_col <= slave_i.adr(ColBits-2 downto 0);
        req_bank <= adr_bank;
        req_row  <= adr_row;
        req_row_hit <= row_hit;
      end if;

      case state is
        when ST_INIT_WAIT =>
          if wait_count = 0 then
            state <= ST_INIT_PRECHARGE;
          else
            wait_count <= wait_count-1;
          end if;

        when ST_INIT_PRECHARGE =>
          state <= ST_INIT_PRECHARGE_WAIT;
          wait_count <= RpCycles-1;

        when ST_INIT_PRECHARGE_WAIT =>
          if wait_count = 0 then
            state <= ST_INIT_REFRESH;
          else
            wait_count <= wait_count-1;
          end if;

        when ST_INIT_REFRESH =>
          state <= ST_INIT_REFRESH_WAIT;
          wait_count <= RfcCycles-1;

        when ST_INIT_REFRESH_WAIT =>
          if wait_count = 0 then
            if init_refresh_left = 0 then
              state <= ST_INIT_MODE;
            else
              init_refresh_left <= init_refresh_left-1;
              state <= ST_INIT_REFRESH;
            end if;
          else
            wait_count <= wait_count-1;
          end if;

        when ST_INIT_MODE =>
          state <= ST_INIT_MODE_WAIT;
          wait_count <= T_MRD_Cycles-1;

        when ST_INIT_MODE_WAIT =>
          if wait_count = 0 then
            state <= ST_IDLE;
          else
            wait_count <= wait_count-1;
          end if;

        when ST_IDLE =>
          if refresh_due = '1' then
            refresh_due <= '0';
            refresh_pending <= '1';
            state <= ST_DRAIN;
          elsif accept = '1' then
            if row_hit = '1' and (slave_i.we = '0' or rd_idle = '1') then
              state <= ST_CAS0;
            else
              state <= ST_DRAIN;
            end if;
          end if;

        -- wait until the SDRAM data bus and the timing constraints allow the
        -- next step: a refresh, a row change, or a column command that has to
        -- turn the bus around from read to write
        when ST_DRAIN =>
          if refresh_pending = '1' then
            if rd_idle = '1' and wr_timer = 0 and ras_timer = 0 then
              if row_open = '1' then
                state <= ST_PRECHARGE;
              else
                state <= ST_REFRESH;
              end if;
            end if;
          elsif req_row_hit = '1' then
            if req_we = '0' or rd_idle = '1' then
              state <= ST_CAS0;
            end if;
          elsif rd_idle = '1' and wr_timer = 0 and ras_timer = 0 then
            start_request;
          end if;

        when ST_PRECHARGE =>
          row_open <= '0';
          if RpCycles > 1 then
            state <= ST_PRECHARGE_WAIT;
            wait_count <= RpWait;
          else
            after_precharge;
          end if;

        when ST_PRECHARGE_WAIT =>
          if wait_count = 0 then
            after_precharge;
          else
            wait_count <= wait_count-1;
          end if;

        when ST_ACTIVATE =>
          row_open <= '1';
          open_bank <= req_bank;
          open_row  <= req_row;
          req_row_hit <= '1';
          ras_timer <= sub_clip(RasCycles,1);
          if RcdCycles > 1 then
            state <= ST_ACTIVATE_WAIT;
            wait_count <= RcdWait;
          else
            state <= ST_CAS0;
          end if;

        when ST_ACTIVATE_WAIT =>
          if wait_count = 0 then
            state <= ST_CAS0;
          else
            wait_count <= wait_count-1;
          end if;

        when ST_CAS0 =>
          if req_we = '1' then
            wr_timer <= sub_clip(WrCycles,1);
          end if;
          state <= ST_CAS1;

        when ST_CAS1 =>
          if req_we = '1' then
            wr_timer <= sub_clip(WrCycles,1);
            ack <= '1';
          end if;
          if accept = '1' then
            -- a write may only follow once the read data pipeline has run dry
            -- (bus turnaround); rd_idle is always '1' behind a write burst
            if row_hit = '1' and (slave_i.we = '0' or rd_idle = '1') then
              state <= ST_CAS0;
            else
              state <= ST_DRAIN;
            end if;
          else
            state <= ST_IDLE;
          end if;

        when ST_REFRESH =>
          state <= ST_REFRESH_WAIT;
          wait_count <= RfcWait;

        when ST_REFRESH_WAIT =>
          if wait_count = 0 then
            refresh_pending <= '0';
            state <= ST_IDLE;
          else
            wait_count <= wait_count-1;
          end if;

      end case;
    end if;
  end process;

end architecture rtl;
