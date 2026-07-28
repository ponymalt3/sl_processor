library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;
use work.apb_p.all;

entity sl_system is

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
    mem_clk_i : in std_ulogic;     -- same-frequency, ~180 degree phase-shifted for mem blocks
    reset_n_i : in std_ulogic;

    uart_rxd_i : in  std_ulogic;
    uart_txd_o : out std_ulogic;

    sdram_clk_o   : out std_ulogic;
    sdram_cke_o   : out std_ulogic;
    sdram_cs_n_o  : out std_ulogic;
    sdram_ras_n_o : out std_ulogic;
    sdram_cas_n_o : out std_ulogic;
    sdram_we_n_o  : out std_ulogic;
    sdram_ba_o    : out std_ulogic_vector(1 downto 0);
    sdram_addr_o  : out std_ulogic_vector(12 downto 0);
    sdram_dqm_o   : out std_ulogic_vector(1 downto 0);
    sdram_dq_io   : inout std_logic_vector(15 downto 0));

end entity sl_system;

architecture rtl of sl_system is

  signal core_en      : std_ulogic_vector(3 downto 0);
  signal core_reset_n : std_ulogic_vector(3 downto 0);

  signal code_master_in  : wb_master_ifc_in_t;
  signal code_master_out : wb_master_ifc_out_t;
  signal ext_master_in   : wb_master_ifc_in_t;
  signal ext_master_out  : wb_master_ifc_out_t;

  signal debug_master_in  : wb_master_ifc_in_t;
  signal debug_master_out : wb_master_ifc_out_t;

  signal master_in  : wb_master_ifc_in_array_t(2 downto 0);
  signal master_out : wb_master_ifc_out_array_t(2 downto 0);
  signal slave_in    : wb_slave_ifc_in_array_t(2 downto 0);
  signal slave_out   : wb_slave_ifc_out_array_t(2 downto 0);

  signal abp_slave_in  : wb_slave_ifc_in_t;
  signal abp_slave_out : wb_slave_ifc_out_t;

  constant SdramCacheWordsPerLine : natural := 8;
  constant SdramCacheLines : natural := (SdramCacheSizeInKB*1024)/(SdramCacheWordsPerLine*4);

  signal sdram_cache_slave_in   : wb_slave_ifc_in_t;
  signal sdram_cache_slave_out  : wb_slave_ifc_out_t;
  signal sdram_cache_master_in  : wb_master_ifc_in_t;
  signal sdram_cache_master_out : wb_master_ifc_out_t;

  signal sdram_cache_addr     : unsigned(31 downto 0);
  signal sdram_cache_wdata    : std_ulogic_vector(31 downto 0);
  signal sdram_cache_rdata    : std_ulogic_vector(31 downto 0);
  signal sdram_cache_en       : std_ulogic;
  signal sdram_cache_we       : std_ulogic;
  signal sdram_cache_complete : std_ulogic;
  signal sdram_cache_err      : std_ulogic;

  signal snoop_active : std_ulogic;

  constant ExtMemSize  : natural := (ExtMemSizeInKB*1024)/4;
  constant SyncMemSize : natural := (SyncMemSizeInKB*1024)/4;
  constant AbpBridgeSize  : natural := 256;
  constant PeripheralSizeInKB : natural := ((SyncMemSize+AbpBridgeSize)*4)/1024;

  signal sync_mem_slave_in  : wb_slave_ifc_in_t;
  signal sync_mem_slave_out : wb_slave_ifc_out_t;

  -- placeholder peripheral entry, not wired to anything real yet (e.g. a
  -- future wb_sync_slave once its semantics are settled)
  constant DebugPeripherals : apb_peripheral_list_t(0 to 0) :=
    (0 => (low => x"0000", high => x"00FF"));

  signal apb_sel  : std_ulogic_vector(DebugPeripherals'range);
  signal apb_en   : std_ulogic;
  signal apb_addr : std_ulogic_vector(15 downto 0);
  signal apb_we   : std_ulogic;
  signal apb_din  : apb_data_t(DebugPeripherals'range);
  signal apb_dout : std_ulogic_vector(31 downto 0);

begin  -- architecture rtl

  sl_cluster_1: entity work.sl_cluster
    generic map (
      LocalMemSizeInKB  => LocalMemSizeInKB,
      ExtMemSizeInKB    => ExtMemSizeInKB,
      CodeMemSizeInKB   => CodeMemSizeInKB,
      CodeCacheSizeInKB => CodeCacheSizeInKB,
      DataCacheSizeInKB => DataCacheSizeInKB,
      PeripheralSizeInKB => PeripheralSizeInKB)
    port map (
      clk_i          => clk_i,
      mem_clk_i      => mem_clk_i,
      reset_n_i      => reset_n_i,
      core_en_i      => core_en,
      core_reset_n_i => core_reset_n,
      ext_master_i   => ext_master_in,
      ext_master_o   => ext_master_out,
      code_master_i  => code_master_in,
      code_master_o  => code_master_out,
      snoop_addr_i   => sdram_cache_slave_in.adr,
      snoop_en_i     => snoop_active);

  wb_ixs_1: entity work.wb_ixs
    generic map (
      MasterConfig => (
        wb_master("sdram sync_mem abp"),
        wb_master("sdram"),
        wb_master("sdram sync_mem abp")),
      SlaveMap => (
        wb_slave("sdram",0,ExtMemSize),
        wb_slave("sync_mem",ExtMemSize,SyncMemSize),
        wb_slave("abp",ExtMemSize+SyncMemSize,AbpBridgeSize)))
    port map (
      clk_i       => clk_i,
      reset_n_i   => reset_n_i,
      master_in_i => master_out,
      master_in_o => master_in,
      slave_out_i => slave_out,
      slave_out_o => slave_in);

  master_out(0) <= debug_master_out;
  debug_master_in <= master_in(0);
  master_out(1) <= code_master_out;
  code_master_in <= master_in(1);
  master_out(2) <= ext_master_out;
  ext_master_in <= master_in(2);

  sdram_cache_slave_in <= slave_in(0);
  slave_out(0) <= sdram_cache_slave_out;
  sync_mem_slave_in <= slave_in(1);
  slave_out(1) <= sync_mem_slave_out;
  abp_slave_in <= slave_in(2);
  slave_out(2) <= abp_slave_out;

  snoop_active <= '1' when sdram_cache_slave_in.cyc = '1' and sdram_cache_slave_in.we = '1' and sdram_cache_slave_out.ack = '1' else '0';

  wb_cache_adapter_sdram: entity work.wb_cache_adapter
    generic map (IsConnectedToIXS => true)
    port map (
      clk_i      => clk_i,
      reset_n_i  => reset_n_i,
      addr_o     => sdram_cache_addr,
      din_i      => sdram_cache_rdata,
      dout_o     => sdram_cache_wdata,
      en_o       => sdram_cache_en,
      we_o       => sdram_cache_we,
      complete_i => sdram_cache_complete,
      err_i      => sdram_cache_err,
      slave_i    => sdram_cache_slave_in,
      slave_o    => sdram_cache_slave_out);

  sdram_cache_1: entity work.wb_cache
    generic map (
      WordsPerLine  => SdramCacheWordsPerLine,
      NumberOfLines => SdramCacheLines,
      WriteThrough  => false)
    port map (
      clk_i           => clk_i,
      mem_clk_i       => mem_clk_i,
      reset_n_i       => reset_n_i,
      addr_i          => sdram_cache_addr,
      din_i           => sdram_cache_wdata,
      dout_o          => sdram_cache_rdata,
      en_i            => sdram_cache_en,
      we_i            => sdram_cache_we,
      complete_o      => sdram_cache_complete,
      err_o           => sdram_cache_err,
      snooping_addr_i => to_unsigned(0,32),
      snooping_en_i   => '0',
      master_out_i    => sdram_cache_master_in,
      master_out_o    => sdram_cache_master_out);

  wb_sdram_1: entity work.wb_sdram
    generic map (
      ClockFreqHz => ClockFreqHz)
    port map (
      clk_i           => clk_i,
      reset_n_i       => reset_n_i,
      slave_i         => sdram_cache_master_out,
      slave_o         => sdram_cache_master_in,
      sdram_clk_o     => sdram_clk_o,
      sdram_cke_o     => sdram_cke_o,
      sdram_cs_n_o    => sdram_cs_n_o,
      sdram_ras_n_o   => sdram_ras_n_o,
      sdram_cas_n_o   => sdram_cas_n_o,
      sdram_we_n_o    => sdram_we_n_o,
      sdram_ba_o      => sdram_ba_o,
      sdram_addr_o    => sdram_addr_o,
      sdram_dqm_o     => sdram_dqm_o,
      sdram_dq_io     => sdram_dq_io);

  wb_mem_sync: entity work.wb_mem
    generic map (
      MemSizeInKB => SyncMemSizeInKB)
    port map (
      clk_i     => clk_i,
      mem_clk_i => mem_clk_i,
      reset_n_i => reset_n_i,
      slave0_i  => sync_mem_slave_in,
      slave0_o  => sync_mem_slave_out,
      slave1_o  => open);

  wb_abp_bridge_1: entity work.wb_abp_bridge
    generic map (
      Peripherals => DebugPeripherals)
    port map (
      clk_i      => clk_i,
      reset_n_i  => reset_n_i,
      slave_i => abp_slave_in,
      slave_o => abp_slave_out,
      apb_sel_o  => apb_sel,
      apb_en_o   => apb_en,
      apb_addr_o => apb_addr,
      apb_we_o   => apb_we,
      apb_din_i  => apb_din,
      apb_dout_o => apb_dout);

  apb_din <= (others => (others => '0'));

  wb_debug_ctrl_1: entity work.wb_debug_ctrl
    generic map (
      ClockFreqHz => ClockFreqHz,
      BaudRate    => BaudRate)
    port map (
      clk_i          => clk_i,
      reset_n_i      => reset_n_i,
      uart_rxd_i     => uart_rxd_i,
      uart_txd_o     => uart_txd_o,
      core_en_o      => core_en,
      core_reset_n_o => core_reset_n,
      master_i       => debug_master_in,
      master_o       => debug_master_out);

end architecture rtl;
