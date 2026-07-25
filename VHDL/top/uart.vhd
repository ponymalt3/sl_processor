library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-- minimal 8N1 UART (no vendor primitives, portable across boards)
entity uart is
  generic (
    ClockFreqHz : natural := 50_000_000;
    BaudRate    : natural := 115200);

  port (
    clk_i     : in std_ulogic;
    reset_n_i : in std_ulogic;

    rxd_i : in  std_ulogic;
    txd_o : out std_ulogic;

    tx_data_i : in  std_ulogic_vector(7 downto 0);
    tx_en_i   : in  std_ulogic;
    tx_busy_o : out std_ulogic;

    rx_data_o  : out std_ulogic_vector(7 downto 0);
    rx_valid_o : out std_ulogic);

end entity uart;

architecture rtl of uart is

  constant CyclesPerBit : natural := ClockFreqHz/BaudRate;

  type tx_state_t is (TX_IDLE, TX_START, TX_DATA, TX_STOP);
  signal tx_state : tx_state_t;
  signal tx_count : natural range 0 to CyclesPerBit-1;
  signal tx_bit_idx : natural range 0 to 7;
  signal tx_shift : std_ulogic_vector(7 downto 0);

  type rx_state_t is (RX_IDLE, RX_START, RX_DATA, RX_STOP);
  signal rx_state : rx_state_t;
  signal rx_count : natural range 0 to CyclesPerBit-1;
  signal rx_bit_idx : natural range 0 to 7;
  signal rx_shift : std_ulogic_vector(7 downto 0);
  signal rxd_1d : std_ulogic;
  signal rxd_2d : std_ulogic;

begin

  tx_busy_o <= '0' when tx_state = TX_IDLE else '1';

  process (clk_i, reset_n_i) is
  begin
    if reset_n_i = '0' then
      tx_state <= TX_IDLE;
      tx_count <= 0;
      tx_bit_idx <= 0;
      tx_shift <= (others => '0');
      txd_o <= '1';
    elsif clk_i'event and clk_i = '1' then
      case tx_state is
        when TX_IDLE =>
          txd_o <= '1';
          if tx_en_i = '1' then
            tx_shift <= tx_data_i;
            tx_count <= CyclesPerBit-1;
            tx_state <= TX_START;
          end if;

        when TX_START =>
          txd_o <= '0';
          if tx_count = 0 then
            tx_count <= CyclesPerBit-1;
            tx_bit_idx <= 0;
            tx_state <= TX_DATA;
          else
            tx_count <= tx_count-1;
          end if;

        when TX_DATA =>
          txd_o <= tx_shift(0);
          if tx_count = 0 then
            tx_count <= CyclesPerBit-1;
            tx_shift <= '0' & tx_shift(7 downto 1);
            if tx_bit_idx = 7 then
              tx_state <= TX_STOP;
            else
              tx_bit_idx <= tx_bit_idx+1;
            end if;
          else
            tx_count <= tx_count-1;
          end if;

        when TX_STOP =>
          txd_o <= '1';
          if tx_count = 0 then
            tx_state <= TX_IDLE;
          else
            tx_count <= tx_count-1;
          end if;
      end case;
    end if;
  end process;

  process (clk_i, reset_n_i) is
  begin
    if reset_n_i = '0' then
      rx_state <= RX_IDLE;
      rx_count <= 0;
      rx_bit_idx <= 0;
      rx_shift <= (others => '0');
      rx_data_o <= (others => '0');
      rx_valid_o <= '0';
      rxd_1d <= '1';
      rxd_2d <= '1';
    elsif clk_i'event and clk_i = '1' then
      -- 2-stage synchronizer for the async rx line
      rxd_1d <= rxd_i;
      rxd_2d <= rxd_1d;

      rx_valid_o <= '0';

      case rx_state is
        when RX_IDLE =>
          if rxd_2d = '0' then
            rx_count <= CyclesPerBit/2;
            rx_state <= RX_START;
          end if;

        when RX_START =>
          if rx_count = 0 then
            rx_count <= CyclesPerBit-1;
            rx_bit_idx <= 0;
            rx_state <= RX_DATA;
          else
            rx_count <= rx_count-1;
          end if;

        when RX_DATA =>
          if rx_count = 0 then
            rx_count <= CyclesPerBit-1;
            rx_shift <= rxd_2d & rx_shift(7 downto 1);
            if rx_bit_idx = 7 then
              rx_state <= RX_STOP;
            else
              rx_bit_idx <= rx_bit_idx+1;
            end if;
          else
            rx_count <= rx_count-1;
          end if;

        when RX_STOP =>
          if rx_count = 0 then
            rx_data_o <= rx_shift;
            rx_valid_o <= '1';
            rx_state <= RX_IDLE;
          else
            rx_count <= rx_count-1;
          end if;
      end case;
    end if;
  end process;

end architecture rtl;
