library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;

-- UART-based debug controller: protocol/command set ported from the old
-- VHDL/quartus/top.vhd, kept wire-compatible with the existing Debugger/
-- host tool (SystemControl.h). Owns the UART, drives a wishbone master for
-- memory read/write, and controls the cluster's core enable/reset lines.
entity wb_debug_ctrl is
  generic (
    ClockFreqHz : natural := 50_000_000;
    BaudRate    : natural := 115200);

  port (
    clk_i     : in std_ulogic;
    reset_n_i : in std_ulogic;

    uart_rxd_i : in  std_ulogic;
    uart_txd_o : out std_ulogic;

    core_en_o      : out std_ulogic_vector(3 downto 0);
    core_reset_n_o : out std_ulogic_vector(3 downto 0);

    master_i : in  wb_master_ifc_in_t;
    master_o : out wb_master_ifc_out_t);

end entity wb_debug_ctrl;

architecture rtl of wb_debug_ctrl is

  constant ST_IDLE     : natural := 0;
  constant ST_ADDR     : natural := 1;
  constant ST_LEN      : natural := 2;
  constant ST_ACK      : natural := 3;
  constant ST_DISPATCH : natural := 4;
  constant ST_READ     : natural := 5;
  constant ST_WRITE    : natural := 6;
  constant ST_CORE     : natural := 7;
  constant ST_PING     : natural := 8;

  signal core_en      : std_ulogic_vector(3 downto 0);
  signal core_reset_n : std_ulogic_vector(3 downto 0);

  signal uart_rx_data  : std_ulogic_vector(7 downto 0);
  signal uart_rx_valid : std_ulogic;
  signal uart_tx_data  : std_ulogic_vector(7 downto 0);
  signal uart_tx_en    : std_ulogic;
  signal uart_tx_busy  : std_ulogic;

  signal wb_en       : std_ulogic;
  signal wb_re       : std_ulogic;
  signal wb_we       : std_ulogic;
  signal wb_dout     : std_ulogic_vector(31 downto 0);
  signal wb_addr     : unsigned(31 downto 0);
  signal wb_complete : std_ulogic;

  signal addr   : unsigned(31 downto 0);
  signal len    : unsigned(15 downto 0);
  signal state  : integer;
  signal cmd    : std_ulogic_vector(7 downto 0);
  signal rdata  : std_ulogic_vector(31 downto 0);
  signal rstate : std_ulogic_vector(3 downto 0);
  signal wdata  : std_ulogic_vector(31 downto 0);
  signal wstate : std_ulogic_vector(3 downto 0);

begin  -- architecture rtl

  core_en_o <= core_en;
  core_reset_n_o <= core_reset_n;

  uart_1: entity work.uart
    generic map (
      ClockFreqHz => ClockFreqHz,
      BaudRate    => BaudRate)
    port map (
      clk_i      => clk_i,
      reset_n_i  => reset_n_i,
      rxd_i      => uart_rxd_i,
      txd_o      => uart_txd_o,
      tx_data_i  => uart_tx_data,
      tx_en_i    => uart_tx_en,
      tx_busy_o  => uart_tx_busy,
      rx_data_o  => uart_rx_data,
      rx_valid_o => uart_rx_valid);

  wb_master_1: entity work.wb_master
    port map (
      clk_i        => clk_i,
      reset_n_i    => reset_n_i,
      addr_i       => wb_addr,
      din_i        => rdata,
      dout_o       => wb_dout,
      en_i         => wb_en,
      we_i         => wb_we,
      complete_o   => wb_complete,
      err_o        => open,
      master_out_i => master_i,
      master_out_o => master_o);

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      addr <= to_unsigned(0,32);
      len <= to_unsigned(0,16);
      rdata <= (others => '0');
      rstate <= "0000";
      wdata <= (others => '0');
      wstate <= "0000";
      core_reset_n <= (others => '0');
      core_en <= (others => '0');
      state <= ST_IDLE;
      cmd <= X"00";
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      if uart_rx_valid = '1' and rstate /= "1111" then
        rdata <= rdata(23 downto 0) & uart_rx_data;
        rstate <= rstate(2 downto 0) & '1';
      end if;

      if uart_tx_busy = '0' and wstate /= "0000" then
        wdata <= wdata(23 downto 0) & X"00";
        wstate <= wstate(2 downto 0) & '0';
      end if;

      case state is
        when ST_IDLE =>
          if rstate /= "0000" then
            cmd <= rdata(7 downto 0);
            rstate <= "0000";
            if uart_rx_data(7) = '1' then
              state <= ST_ADDR;
            else
              state <= ST_DISPATCH;
            end if;
          end if;
        when ST_ADDR =>
          if rstate = "1111" then
            addr <= unsigned(rdata);
            state <= ST_LEN;
            rstate <= "0000";
          end if;
        when ST_LEN =>
          if rstate(1 downto 0) = "11" then
            len <= unsigned(rdata(15 downto 0));
            state <= ST_DISPATCH;
            rstate <= "0000";
          end if;
        when ST_DISPATCH =>
          state <= state+to_integer(unsigned(cmd(6 downto 0)));
        when ST_ACK =>
          if wstate = "0000" then
            state <= ST_IDLE;
            wstate <= "1000";
            wdata(31 downto 24) <= cmd;
          end if;
        when ST_CORE =>
          if rstate(1 downto 0) = "11" then
            core_en <= rdata(3 downto 0) and not rdata(11 downto 8);
            core_reset_n <= not rdata(11 downto 8);
            state <= ST_ACK;
            rstate <= "0000";
          end if;
        when ST_PING => state <= ST_ACK;
        when others => null;
      end case;

      if (wb_re = '1' or wb_we = '1') and wb_complete = '1' then
        addr <= addr+1;
        len <= len-1;
        if len = to_unsigned(1,16) then
          state <= ST_ACK;
        end if;
      end if;

      if wb_we = '1' and wb_complete = '1' then
        rstate <= "0000";
      end if;

      if wb_re = '1' and wb_complete = '1' then
        wdata <= wb_dout;
        wstate <= "1111";
      end if;

    end if;
  end process;

  wb_re <= '1' when wstate = "0000" and state = ST_READ else '0';
  wb_we <= '1' when rstate = "1111" and state = ST_WRITE else '0';
  wb_en <= (wb_re or wb_we) and not wb_complete;
  wb_addr <= addr;

  uart_tx_en <= '1' when uart_tx_busy = '0' and wstate /= "0000" else '0';
  uart_tx_data <= wdata(31 downto 24);

end architecture rtl;
