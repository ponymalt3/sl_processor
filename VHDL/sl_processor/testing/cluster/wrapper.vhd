-- Flat-port wrapper around sl_cluster for cocotb.
-- Topology mirrors sl_cluster_tb.vhd (DUT sl_cluster + wb_ixs arbitrating a probe
-- master against the DUT's own code_master onto one physical wb_mem; the DUT's
-- ext_master talks to the same wb_mem directly via a second port, already
-- offset onto the unified code/ext address space by the cluster itself) but
-- the raw VHDL test process is replaced by a wb_master probe exposed as flat
-- signals so cocotb/GHDL-VPI can drive it.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;

entity sl_cluster_wrapper is
  port (
    clk_i          : in  std_ulogic;
    reset_n_i      : in  std_ulogic;
    core_en_i      : in  std_ulogic_vector(3 downto 0);
    core_reset_n_i : in  std_ulogic_vector(3 downto 0);

    -- probe: flat wishbone-master-style port, arbitrated against the DUT's own
    -- code fetch onto the shared physical memory (code at word 0.., ext data at
    -- word CodeMemSizeInKB*1024/4=2048..)
    probe_addr_i     : in  unsigned(31 downto 0);
    probe_din_i      : in  std_ulogic_vector(31 downto 0);
    probe_dout_o     : out std_ulogic_vector(31 downto 0);
    probe_en_i       : in  std_ulogic;
    probe_burst_i    : in  unsigned(5 downto 0);
    probe_we_i       : in  std_ulogic;
    probe_dready_o   : out std_ulogic;
    probe_complete_o : out std_ulogic
  );
end entity sl_cluster_wrapper;

architecture behav of sl_cluster_wrapper is

  signal mem_clk : std_ulogic;

  signal probe_master_in  : wb_master_ifc_in_t;
  signal probe_master_out : wb_master_ifc_out_t;

  signal code_master_in  : wb_master_ifc_in_t;
  signal code_master_out : wb_master_ifc_out_t;

  signal master_in  : wb_master_ifc_in_array_t(1 downto 0);
  signal master_out : wb_master_ifc_out_array_t(1 downto 0);
  signal slave_in    : wb_slave_ifc_in_array_t(0 downto 0);
  signal slave_out   : wb_slave_ifc_out_array_t(0 downto 0);

  signal ext_master_in   : wb_master_ifc_in_t;
  signal ext_master_out  : wb_master_ifc_out_t;
  signal ext_master_out2 : wb_master_ifc_out_t;

begin

  mem_clk <= not clk_i;

  DUT : entity work.sl_cluster
    generic map (
      LocalMemSizeInKB  => 2,
      ExtMemSizeInKB    => 16,
      CodeMemSizeInKB   => 8,
      CodeCacheSizeInKB => 1,
      DataCacheSizeInKB => 1)
    port map (
      clk_i          => clk_i,
      mem_clk_i      => mem_clk,
      reset_n_i      => reset_n_i,
      core_en_i      => core_en_i,
      core_reset_n_i => core_reset_n_i,
      ext_master_i   => ext_master_in,
      ext_master_o   => ext_master_out,
      code_master_i  => code_master_in,
      code_master_o  => code_master_out);

  wb_master_1 : entity work.wb_master
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
      master_out_i => master_in(0),
      master_out_o => master_out(0));

  master_out(1) <= code_master_out;
  code_master_in <= master_in(1);

  wb_ixs_1 : entity work.wb_ixs
    generic map (
      MasterConfig => (
        wb_master("mem"),
        wb_master("mem")),
      SlaveMap => (
        0 => wb_slave("mem", 0, 4096)))
    port map (
      clk_i       => clk_i,
      reset_n_i   => reset_n_i,
      master_in_i => master_out,
      master_in_o => master_in,
      slave_out_i => slave_out,
      slave_out_o => slave_in);

  wb_mem_1 : entity work.wb_mem
    generic map (
      MemSizeInKB => 24)
    port map (
      clk_i     => clk_i,
      mem_clk_i => mem_clk,
      reset_n_i => reset_n_i,
      slave0_i  => ext_master_out2,
      slave0_o  => ext_master_in,
      slave1_i  => slave_in(0),
      slave1_o  => slave_out(0));

  -- sl_cluster now offsets ext_master_o onto the unified code/ext address
  -- space itself (code_mem and ext_mem are the same underlying memory), so
  -- no additional offset is needed here.
  ext_master_out2 <= ext_master_out;

end architecture behav;
