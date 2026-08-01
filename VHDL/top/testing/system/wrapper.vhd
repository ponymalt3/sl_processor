-- Cocotb-only wrapper around the real sl_system (top.vhd) entity.
-- Forwards every port straight through except sdram_dq_io: GHDL's VPI
-- doesn't resolve a value cocotb writes to an inout port against the
-- port's own RTL driver (see sdram_model.py's docstring), so the model's
-- tri-state mux is done here in VHDL instead, as a second real driver on
-- the internal dq_bus net -- cocotb only touches the plain, single-driver
-- sdram_model_* ports. mem_clk_i is generated here as "not clk_i" so the
-- test side only drives one clock (matches wb_cache_wrapper.vhd).
-- top.vhd itself is untouched.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity sl_system_tb_wrapper is
  generic (
    ClockFreqHz       : natural := 50_000_000;
    BaudRate          : natural := 1000000;
    LocalMemSizeInKB  : natural := 3;
    CodeMemSizeInKB   : natural := 64;
    ExtMemSizeInKB    : natural := 8*1024;
    CodeCacheSizeInKB : natural := 2;
    DataCacheSizeInKB : natural := 4;
    SdramCacheSizeInKB : natural := 32;
    SyncMemSizeInKB    : natural := 2);
  port (
    clk_i     : in std_ulogic;
    reset_n_i : in std_ulogic;

    uart_rxd_i : in  std_ulogic;
    uart_txd_o : out std_ulogic;

    sdram_cs_n_o  : out std_ulogic;
    sdram_ras_n_o : out std_ulogic;
    sdram_cas_n_o : out std_ulogic;
    sdram_we_n_o  : out std_ulogic;
    sdram_ba_o    : out std_ulogic_vector(1 downto 0);
    sdram_addr_o  : out std_ulogic_vector(12 downto 0);
    sdram_dqm_o   : out std_ulogic_vector(1 downto 0);

    sdram_model_din_o  : out std_ulogic_vector(15 downto 0);  -- current bus value
    sdram_model_dout_i : in  std_ulogic_vector(15 downto 0);  -- model's drive data
    sdram_model_oe_i   : in  std_ulogic);
end entity sl_system_tb_wrapper;

architecture rtl of sl_system_tb_wrapper is

  signal dq_bus  : std_logic_vector(15 downto 0);
  signal mem_clk : std_ulogic;

begin

  mem_clk <= not clk_i;

  dq_bus <= To_StdLogicVector(sdram_model_dout_i) when sdram_model_oe_i = '1'
            else (others => 'Z');
  sdram_model_din_o <= To_StdULogicVector(dq_bus);

  sl_system_1: entity work.sl_system
    generic map (
      ClockFreqHz        => ClockFreqHz,
      BaudRate           => BaudRate,
      LocalMemSizeInKB   => LocalMemSizeInKB,
      CodeMemSizeInKB    => CodeMemSizeInKB,
      ExtMemSizeInKB     => ExtMemSizeInKB,
      CodeCacheSizeInKB  => CodeCacheSizeInKB,
      DataCacheSizeInKB  => DataCacheSizeInKB,
      SdramCacheSizeInKB => SdramCacheSizeInKB,
      SyncMemSizeInKB    => SyncMemSizeInKB)
    port map (
      clk_i         => clk_i,
      mem_clk_i     => mem_clk,
      reset_n_i     => reset_n_i,
      uart_rxd_i    => uart_rxd_i,
      uart_txd_o    => uart_txd_o,
      sdram_clk_o   => open,
      sdram_cke_o   => open,
      sdram_cs_n_o  => sdram_cs_n_o,
      sdram_ras_n_o => sdram_ras_n_o,
      sdram_cas_n_o => sdram_cas_n_o,
      sdram_we_n_o  => sdram_we_n_o,
      sdram_ba_o    => sdram_ba_o,
      sdram_addr_o  => sdram_addr_o,
      sdram_dqm_o   => sdram_dqm_o,
      sdram_dq_io   => dq_bus);

end architecture rtl;
