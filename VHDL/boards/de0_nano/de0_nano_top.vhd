library ieee;
use ieee.std_logic_1164.all;

-- DE0-Nano board-level top: physical pins + PLL only. All actual system
-- logic lives in the portable work.sl_system (VHDL/top/top.vhd); this file
-- and pll.vhd are the only Altera/Cyclone-IV-specific pieces, kept separate
-- on purpose so sl_system can be retargeted to another board later.
entity de0_nano_top is
  port (
    CLOCK_50 : in    std_logic;
    KEY      : in    std_logic_vector(1 downto 0);
    SW       : in    std_logic_vector(3 downto 0);
    LED      : out   std_logic_vector(7 downto 0);

    DRAM_ADDR  : out   std_logic_vector(12 downto 0);
    DRAM_BA    : out   std_logic_vector(1 downto 0);
    DRAM_CAS_N : out   std_logic;
    DRAM_CKE   : out   std_logic;
    DRAM_CLK   : out   std_logic;
    DRAM_CS_N  : out   std_logic;
    DRAM_DQ    : inout std_logic_vector(15 downto 0);
    DRAM_DQM   : out   std_logic_vector(1 downto 0);
    DRAM_RAS_N : out   std_logic;
    DRAM_WE_N  : out   std_logic;

    -- GPIO_0(0)/(1) carry the debug UART (RxD/TxD); rest unused here
    GPIO_0 : inout std_logic_vector(33 downto 0));

end entity de0_nano_top;

architecture rtl of de0_nano_top is

  signal clk               : std_ulogic;
  signal mem_clk           : std_ulogic;
  signal sdram_clk_shifted : std_ulogic;
  signal reset_n           : std_ulogic;

  signal uart_rxd : std_ulogic;
  signal uart_txd : std_ulogic;

  signal sdram_cke   : std_ulogic;
  signal sdram_cs_n  : std_ulogic;
  signal sdram_ras_n : std_ulogic;
  signal sdram_cas_n : std_ulogic;
  signal sdram_we_n  : std_ulogic;
  signal sdram_ba    : std_ulogic_vector(1 downto 0);
  signal sdram_addr  : std_ulogic_vector(12 downto 0);
  signal sdram_dqm   : std_ulogic_vector(1 downto 0);

  signal debug : std_ulogic_vector(7 downto 0);

begin

  pll_1: entity work.pll
    port map (
      inclk0 => CLOCK_50,
      c0     => clk,
      c1     => sdram_clk_shifted,
      c2     => mem_clk);

  reset_n <= KEY(0);

  sl_system_1: entity work.sl_system
    generic map (
      -- EP4CE22 only has 66 M9K blocks (~594Kbit/~74KB) of on-chip block
      -- RAM total; sl_system's own defaults (32KB L2 + 4x2KB code/data
      -- caches) alone are right at that ceiling with zero packing margin,
      -- so this board sizes down to comfortably fit alongside everything
      -- else (local mem, tag storage, sync mem)
      CodeCacheSizeInKB => 1,
      DataCacheSizeInKB => 1,
      SdramCacheSizeInKB => 8,
      SyncMemSizeInKB    => 4)
    port map (
      clk_i         => clk,
      mem_clk_i     => mem_clk,
      reset_n_i     => reset_n,
      uart_rxd_i    => uart_rxd,
      uart_txd_o    => uart_txd,
      sdram_clk_o   => open,          -- physical DRAM_CLK comes straight
                                       -- from the PLL's phase-shifted c1,
                                       -- not from sl_system/wb_sdram
      sdram_cke_o   => sdram_cke,
      sdram_cs_n_o  => sdram_cs_n,
      sdram_ras_n_o => sdram_ras_n,
      sdram_cas_n_o => sdram_cas_n,
      sdram_we_n_o  => sdram_we_n,
      sdram_ba_o    => sdram_ba,
      sdram_addr_o  => sdram_addr,
      sdram_dqm_o   => sdram_dqm,
      sdram_dq_io   => DRAM_DQ,
      debug_o       => debug);

  DRAM_CLK   <= sdram_clk_shifted;
  DRAM_CKE   <= sdram_cke;
  DRAM_CS_N  <= sdram_cs_n;
  DRAM_RAS_N <= sdram_ras_n;
  DRAM_CAS_N <= sdram_cas_n;
  DRAM_WE_N  <= sdram_we_n;
  DRAM_BA    <= std_logic_vector(sdram_ba);
  DRAM_ADDR  <= std_logic_vector(sdram_addr);
  DRAM_DQM   <= std_logic_vector(sdram_dqm);

  uart_rxd   <= GPIO_0(0);
  GPIO_0(1)  <= uart_txd;

  LED <= std_logic_vector(debug);

end architecture rtl;
