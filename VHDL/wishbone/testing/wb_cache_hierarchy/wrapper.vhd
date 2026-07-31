-- Multi-level cache stress-test topology: 8 L1 write-through caches (mirrors
-- sl_cluster.vhd's 4 cores x code+data cache) feeding through a nested pair
-- of wb_ixs arbiters into a single L2 write-back cache (mirrors top.vhd's
-- sdram_cache_1), backed by plain wb_mem. A "probe" master gives direct L2
-- access alongside the 8 L1s (mirrors wb_debug_ctrl.vhd's direct path), and
-- every L2 write broadcasts a snoop invalidation to all 8 L1s, exactly like
-- top.vhd's `snoop_active` signal. Exists to validate the caching/snooping
-- subsystem under many parallel/varied access patterns, independent of the
-- processor pipeline.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;

entity wb_cache_hierarchy_wrapper is
  port (
    clk_i     : in std_ulogic;
    reset_n_i : in std_ulogic;

    l1_0_addr_i : in unsigned(31 downto 0); l1_0_din_i : in std_ulogic_vector(31 downto 0);
    l1_0_dout_o : out std_ulogic_vector(31 downto 0); l1_0_en_i : in std_ulogic;
    l1_0_we_i : in std_ulogic; l1_0_complete_o : out std_ulogic;

    l1_1_addr_i : in unsigned(31 downto 0); l1_1_din_i : in std_ulogic_vector(31 downto 0);
    l1_1_dout_o : out std_ulogic_vector(31 downto 0); l1_1_en_i : in std_ulogic;
    l1_1_we_i : in std_ulogic; l1_1_complete_o : out std_ulogic;

    l1_2_addr_i : in unsigned(31 downto 0); l1_2_din_i : in std_ulogic_vector(31 downto 0);
    l1_2_dout_o : out std_ulogic_vector(31 downto 0); l1_2_en_i : in std_ulogic;
    l1_2_we_i : in std_ulogic; l1_2_complete_o : out std_ulogic;

    l1_3_addr_i : in unsigned(31 downto 0); l1_3_din_i : in std_ulogic_vector(31 downto 0);
    l1_3_dout_o : out std_ulogic_vector(31 downto 0); l1_3_en_i : in std_ulogic;
    l1_3_we_i : in std_ulogic; l1_3_complete_o : out std_ulogic;

    l1_4_addr_i : in unsigned(31 downto 0); l1_4_din_i : in std_ulogic_vector(31 downto 0);
    l1_4_dout_o : out std_ulogic_vector(31 downto 0); l1_4_en_i : in std_ulogic;
    l1_4_we_i : in std_ulogic; l1_4_complete_o : out std_ulogic;

    l1_5_addr_i : in unsigned(31 downto 0); l1_5_din_i : in std_ulogic_vector(31 downto 0);
    l1_5_dout_o : out std_ulogic_vector(31 downto 0); l1_5_en_i : in std_ulogic;
    l1_5_we_i : in std_ulogic; l1_5_complete_o : out std_ulogic;

    l1_6_addr_i : in unsigned(31 downto 0); l1_6_din_i : in std_ulogic_vector(31 downto 0);
    l1_6_dout_o : out std_ulogic_vector(31 downto 0); l1_6_en_i : in std_ulogic;
    l1_6_we_i : in std_ulogic; l1_6_complete_o : out std_ulogic;

    l1_7_addr_i : in unsigned(31 downto 0); l1_7_din_i : in std_ulogic_vector(31 downto 0);
    l1_7_dout_o : out std_ulogic_vector(31 downto 0); l1_7_en_i : in std_ulogic;
    l1_7_we_i : in std_ulogic; l1_7_complete_o : out std_ulogic;

    -- direct L2 access, alongside the 8 L1s (mirrors wb_debug_ctrl.vhd)
    probe_addr_i     : in  unsigned(31 downto 0);
    probe_din_i      : in  std_ulogic_vector(31 downto 0);
    probe_dout_o     : out std_ulogic_vector(31 downto 0);
    probe_en_i       : in  std_ulogic;
    probe_burst_i    : in  unsigned(5 downto 0);
    probe_we_i       : in  std_ulogic;
    probe_dready_o   : out std_ulogic;
    probe_complete_o : out std_ulogic
  );
end entity wb_cache_hierarchy_wrapper;

architecture rtl of wb_cache_hierarchy_wrapper is

  constant NumL1 : natural := 8;
  constant SlaveSize : natural := 1024;  -- must be a power of 2 (wb_ixs_decoder masks, doesn't subtract)

  signal mem_clk : std_ulogic;

  type addr_array_t is array (0 to NumL1-1) of unsigned(31 downto 0);
  type data_array_t is array (0 to NumL1-1) of std_ulogic_vector(31 downto 0);

  signal l1_addr : addr_array_t;
  signal l1_din  : data_array_t;
  signal l1_dout : data_array_t;
  signal l1_en   : std_ulogic_vector(0 to NumL1-1);
  signal l1_we   : std_ulogic_vector(0 to NumL1-1);
  signal l1_complete : std_ulogic_vector(0 to NumL1-1);

  signal l1_master_out : wb_master_ifc_out_array_t(0 to NumL1-1);
  signal l1_master_in  : wb_master_ifc_in_array_t(0 to NumL1-1);

  -- inner IXS: 8 L1 masters -> 1 slave ("l2")
  signal inner_slave_in  : wb_slave_ifc_in_array_t(0 downto 0);
  signal inner_slave_out : wb_slave_ifc_out_array_t(0 downto 0);

  -- outer IXS: inner-group master + probe master -> 1 slave ("l2")
  signal outer_master_out : wb_master_ifc_out_array_t(1 downto 0);
  signal outer_master_in  : wb_master_ifc_in_array_t(1 downto 0);
  signal outer_slave_in   : wb_slave_ifc_in_array_t(0 downto 0);
  signal outer_slave_out  : wb_slave_ifc_out_array_t(0 downto 0);

  signal probe_master_in  : wb_master_ifc_in_t;
  signal probe_master_out : wb_master_ifc_out_t;

  signal l2_addr     : unsigned(31 downto 0);
  signal l2_wdata     : std_ulogic_vector(31 downto 0);
  signal l2_rdata    : std_ulogic_vector(31 downto 0);
  signal l2_en       : std_ulogic;
  signal l2_we       : std_ulogic;
  signal l2_complete : std_ulogic;
  signal l2_err      : std_ulogic;

  signal l2_master_in  : wb_master_ifc_in_t;
  signal l2_master_out : wb_master_ifc_out_t;

  signal snoop_active : std_ulogic;
  signal snoop_addr   : unsigned(31 downto 0);

begin

  mem_clk <= not clk_i;

  -- pack/unpack the 8 named L1 ports into arrays for the generate loop below
  l1_addr <= (l1_0_addr_i, l1_1_addr_i, l1_2_addr_i, l1_3_addr_i,
              l1_4_addr_i, l1_5_addr_i, l1_6_addr_i, l1_7_addr_i);
  l1_din  <= (l1_0_din_i, l1_1_din_i, l1_2_din_i, l1_3_din_i,
              l1_4_din_i, l1_5_din_i, l1_6_din_i, l1_7_din_i);
  l1_en   <= (l1_0_en_i, l1_1_en_i, l1_2_en_i, l1_3_en_i,
              l1_4_en_i, l1_5_en_i, l1_6_en_i, l1_7_en_i);
  l1_we   <= (l1_0_we_i, l1_1_we_i, l1_2_we_i, l1_3_we_i,
              l1_4_we_i, l1_5_we_i, l1_6_we_i, l1_7_we_i);

  l1_0_dout_o <= l1_dout(0); l1_0_complete_o <= l1_complete(0);
  l1_1_dout_o <= l1_dout(1); l1_1_complete_o <= l1_complete(1);
  l1_2_dout_o <= l1_dout(2); l1_2_complete_o <= l1_complete(2);
  l1_3_dout_o <= l1_dout(3); l1_3_complete_o <= l1_complete(3);
  l1_4_dout_o <= l1_dout(4); l1_4_complete_o <= l1_complete(4);
  l1_5_dout_o <= l1_dout(5); l1_5_complete_o <= l1_complete(5);
  l1_6_dout_o <= l1_dout(6); l1_6_complete_o <= l1_complete(6);
  l1_7_dout_o <= l1_dout(7); l1_7_complete_o <= l1_complete(7);

  l1_gen: for i in 0 to NumL1-1 generate
  begin
    l1_cache: entity work.wb_cache
      generic map (
        WordsPerLine  => 4,
        NumberOfLines => 8,
        WriteThrough  => true,
        NarrowTag     => true)
      port map (
        clk_i           => clk_i,
        mem_clk_i       => mem_clk,
        reset_n_i       => reset_n_i,
        addr_i          => l1_addr(i),
        din_i           => l1_din(i),
        dout_o          => l1_dout(i),
        en_i            => l1_en(i),
        we_i            => l1_we(i),
        complete_o      => l1_complete(i),
        err_o           => open,
        snooping_addr_i => snoop_addr,
        snooping_en_i   => snoop_active,
        master_out_i    => l1_master_in(i),
        master_out_o    => l1_master_out(i));
  end generate l1_gen;

  inner_ixs: entity work.wb_ixs
    generic map (
      MasterConfig => (
        wb_master("l2"), wb_master("l2"), wb_master("l2"), wb_master("l2"),
        wb_master("l2"), wb_master("l2"), wb_master("l2"), wb_master("l2")),
      SlaveMap => (0 => wb_slave("l2", 0, SlaveSize)))
    port map (
      clk_i       => clk_i,
      reset_n_i   => reset_n_i,
      master_in_i => l1_master_out,
      master_in_o => l1_master_in,
      slave_out_i => inner_slave_out,
      slave_out_o => inner_slave_in);

  -- inner IXS's single slave-side output plugs directly into the outer IXS
  -- as one master, same as sl_cluster.vhd's ext_master_o feeding top.vhd
  outer_master_out(0) <= inner_slave_in(0);
  inner_slave_out(0)  <= outer_master_in(0);

  probe_master: entity work.wb_master
    port map (
      clk_i        => clk_i,
      reset_n_i    => reset_n_i,
      addr_i       => probe_addr_i,
      din_i        => probe_din_i,
      dout_o       => probe_dout_o,
      en_i         => probe_en_i,
      burst_i      => probe_burst_i,
      we_i         => probe_we_i,
      dready_o     => probe_dready_o,
      complete_o   => probe_complete_o,
      err_o        => open,
      master_out_i => outer_master_in(1),
      master_out_o => outer_master_out(1));

  outer_ixs: entity work.wb_ixs
    generic map (
      MasterConfig => (wb_master("l2"), wb_master("l2")),
      SlaveMap => (0 => wb_slave("l2", 0, SlaveSize)))
    port map (
      clk_i       => clk_i,
      reset_n_i   => reset_n_i,
      master_in_i => outer_master_out,
      master_in_o => outer_master_in,
      slave_out_i => outer_slave_out,
      slave_out_o => outer_slave_in);

  -- every write that completes through the L2 cache broadcasts a snoop
  -- invalidation to all 8 L1s -- mirrors top.vhd's snoop_active exactly
  snoop_active <= '1' when outer_slave_in(0).cyc = '1' and outer_slave_in(0).we = '1'
                        and outer_slave_out(0).ack = '1' else '0';
  snoop_addr <= outer_slave_in(0).adr;

  wb_cache_adapter_l2: entity work.wb_cache_adapter
    generic map (IsConnectedToIXS => true)
    port map (
      clk_i      => clk_i,
      reset_n_i  => reset_n_i,
      addr_o     => l2_addr,
      din_i      => l2_rdata,
      dout_o     => l2_wdata,
      en_o       => l2_en,
      we_o       => l2_we,
      complete_i => l2_complete,
      err_i      => l2_err,
      slave_i    => outer_slave_in(0),
      slave_o    => outer_slave_out(0));

  l2_cache: entity work.wb_cache
    generic map (
      WordsPerLine  => 8,
      NumberOfLines => 16,
      WriteThrough  => false)
    port map (
      clk_i           => clk_i,
      mem_clk_i       => mem_clk,
      reset_n_i       => reset_n_i,
      addr_i          => l2_addr,
      din_i           => l2_wdata,
      dout_o          => l2_rdata,
      en_i            => l2_en,
      we_i            => l2_we,
      complete_o      => l2_complete,
      err_o           => l2_err,
      snooping_addr_i => to_unsigned(0, 32),
      snooping_en_i   => '0',
      master_out_i    => l2_master_in,
      master_out_o    => l2_master_out);

  wb_mem_1: entity work.wb_mem
    generic map (
      MemSizeInKB => 4)
    port map (
      clk_i     => clk_i,
      mem_clk_i => mem_clk,
      reset_n_i => reset_n_i,
      slave0_i  => l2_master_out,
      slave0_o  => l2_master_in,
      slave1_o  => open);

end architecture rtl;
