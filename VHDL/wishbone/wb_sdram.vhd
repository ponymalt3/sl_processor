library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

use work.wishbone_p.all;

-- Wishbone-native SDR SDRAM controller, defaults matched to the DE0-Nano's
-- 32MB x16 4-bank SDRAM at 50MHz (13-bit row, 9-bit column). Simple design:
-- every access opens its row, transfers, auto-precharges and fully closes
-- again before the next command -- no row kept open across accesses.
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

  constant InitCycles      : natural := natural(ceil(InitDelayUs*1000.0/ClockPeriodNs));
  constant RcdCycles       : natural := natural(ceil(T_RCD_Ns/ClockPeriodNs));
  constant RpCycles        : natural := natural(ceil(T_RP_Ns/ClockPeriodNs));
  constant RfcCycles       : natural := natural(ceil(T_RFC_Ns/ClockPeriodNs));
  constant WrCycles        : natural := natural(ceil(T_WR_Ns/ClockPeriodNs));
  constant RefreshCycles   : natural := natural(RefreshIntervalUs*1000.0/ClockPeriodNs);
  constant CloseCyclesRead : natural := RpCycles;
  constant CloseCyclesWrite : natural := WrCycles+RpCycles;

  constant CasLatencyCode : std_ulogic_vector(2 downto 0) := std_ulogic_vector(to_unsigned(CasLatency,3));
  constant ModeRegVal : std_ulogic_vector(12 downto 0) := "000" & "0" & "00" & CasLatencyCode & "0" & "000";

  type state_t is (
    ST_INIT_WAIT, ST_INIT_PRECHARGE, ST_INIT_PRECHARGE_WAIT,
    ST_INIT_REFRESH, ST_INIT_REFRESH_WAIT, ST_INIT_MODE, ST_INIT_MODE_WAIT,
    ST_IDLE, ST_REFRESH, ST_REFRESH_WAIT,
    ST_ACTIVATE, ST_ACTIVATE_WAIT,
    ST_CAS0, ST_CAS0_WAIT, ST_CAS1, ST_CAS1_WAIT,
    ST_ROW_CLOSE);

  signal state : state_t;
  signal wait_count : natural range 0 to InitCycles-1;
  signal init_refresh_left : natural range 0 to InitRefreshCount-1;
  signal refresh_timer : natural range 0 to RefreshCycles-1;
  signal refresh_due : std_ulogic;

  signal latched_we  : std_ulogic;
  signal latched_sel : std_ulogic_vector(3 downto 0);
  signal latched_din : std_ulogic_vector(31 downto 0);
  signal latched_bank : unsigned(BankBits-1 downto 0);
  signal latched_row  : unsigned(RowBits-1 downto 0);
  signal latched_col_base : unsigned(ColBits-2 downto 0);

  signal read_low  : std_ulogic_vector(15 downto 0);
  signal read_data : std_ulogic_vector(31 downto 0);
  signal ack : std_ulogic;

  signal dq_out : std_ulogic_vector(15 downto 0);
  signal dq_oe  : std_ulogic;
  signal stall  : std_ulogic;

begin

  sdram_clk_o <= clk_i;
  sdram_cke_o <= '1';
  sdram_dq_io <= To_StdLogicVector(dq_out) when dq_oe = '1' else (others => 'Z');

  stall <= '0' when (state = ST_IDLE and refresh_due = '0') else '1';
  slave_o <= (read_data,ack,'0',stall);

  -- command / datapath output decode (Moore: depends only on current state
  -- and the registered request latched when leaving ST_IDLE)
  process (state, latched_we, latched_sel, latched_din, latched_bank,
           latched_row, latched_col_base) is
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
      when ST_INIT_PRECHARGE =>
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
        sdram_ba_o <= std_ulogic_vector(latched_bank);
        sdram_addr_o(RowBits-1 downto 0) <= std_ulogic_vector(latched_row);

      when ST_CAS0 | ST_CAS1 =>
        sdram_cas_n_o <= '0';
        sdram_we_n_o  <= not latched_we;
        sdram_ba_o <= std_ulogic_vector(latched_bank);
        sdram_addr_o(ColBits-1 downto 1) <= std_ulogic_vector(latched_col_base);
        if state = ST_CAS0 then
          sdram_addr_o(0)  <= '0';
          sdram_addr_o(10) <= '0';
        else
          sdram_addr_o(0)  <= '1';
          sdram_addr_o(10) <= '1';       -- auto-precharge on 2nd half
        end if;
        if latched_we = '1' then
          dq_oe <= '1';
          if state = ST_CAS0 then
            dq_out <= latched_din(15 downto 0);
            sdram_dqm_o <= not latched_sel(1 downto 0);
          else
            dq_out <= latched_din(31 downto 16);
            sdram_dqm_o <= not latched_sel(3 downto 2);
          end if;
        end if;

      when others => null;             -- NOP
    end case;
  end process;

  process (clk_i, reset_n_i) is
  begin
    if reset_n_i = '0' then
      state <= ST_INIT_WAIT;
      wait_count <= InitCycles-1;
      init_refresh_left <= InitRefreshCount-1;
      refresh_timer <= 0;
      refresh_due <= '0';
      latched_we <= '0';
      latched_sel <= (others => '0');
      latched_din <= (others => '0');
      latched_bank <= (others => '0');
      latched_row <= (others => '0');
      latched_col_base <= (others => '0');
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
            state <= ST_REFRESH;
          elsif slave_i.cyc = '1' and slave_i.stb = '1' then
            latched_we  <= slave_i.we;
            latched_sel <= slave_i.sel;
            latched_din <= slave_i.dat;
            latched_col_base <= slave_i.adr(ColBits-2 downto 0);
            latched_bank <= slave_i.adr(ColBits+BankBits-2 downto ColBits-1);
            latched_row  <= slave_i.adr(ColBits+BankBits+RowBits-2 downto ColBits+BankBits-1);
            state <= ST_ACTIVATE;
          end if;

        when ST_REFRESH =>
          state <= ST_REFRESH_WAIT;
          wait_count <= RfcCycles-1;

        when ST_REFRESH_WAIT =>
          if wait_count = 0 then
            state <= ST_IDLE;
          else
            wait_count <= wait_count-1;
          end if;

        when ST_ACTIVATE =>
          state <= ST_ACTIVATE_WAIT;
          wait_count <= RcdCycles-1;

        when ST_ACTIVATE_WAIT =>
          if wait_count = 0 then
            state <= ST_CAS0;
          else
            wait_count <= wait_count-1;
          end if;

        when ST_CAS0 =>
          if latched_we = '1' then
            state <= ST_CAS1;
          else
            state <= ST_CAS0_WAIT;
            wait_count <= CasLatency-1;
          end if;

        when ST_CAS0_WAIT =>
          if wait_count = 0 then
            read_low <= To_StdULogicVector(sdram_dq_io);
            state <= ST_CAS1;
          else
            wait_count <= wait_count-1;
          end if;

        when ST_CAS1 =>
          if latched_we = '1' then
            ack <= '1';
            state <= ST_ROW_CLOSE;
            wait_count <= CloseCyclesWrite-1;
          else
            state <= ST_CAS1_WAIT;
            wait_count <= CasLatency-1;
          end if;

        when ST_CAS1_WAIT =>
          if wait_count = 0 then
            read_data <= To_StdULogicVector(sdram_dq_io) & read_low;
            ack <= '1';
            state <= ST_ROW_CLOSE;
            wait_count <= CloseCyclesRead-1;
          else
            wait_count <= wait_count-1;
          end if;

        when ST_ROW_CLOSE =>
          if wait_count = 0 then
            state <= ST_IDLE;
          else
            wait_count <= wait_count-1;
          end if;

      end case;
    end if;
  end process;

end architecture rtl;
