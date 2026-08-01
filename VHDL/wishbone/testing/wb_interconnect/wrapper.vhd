-- Flat-port wrapper around wb_ixs for cocotb testing.
-- Includes 3 wb_master components, the wb_ixs interconnect and 3 simple VHDL
-- slave processes — matching the original wb_interconnect_tb.vhd topology.
-- Python drives master high-level ports; slaves are handled natively in VHDL.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;

entity wb_ixs_wrapper is
  port (
    clk_i     : in std_ulogic;
    reset_n_i : in std_ulogic;

    -- Master 0: slv1 + mem0 + mem2
    m0_addr_i     : in  unsigned(31 downto 0);
    m0_din_i      : in  std_ulogic_vector(31 downto 0);
    m0_dout_o     : out std_ulogic_vector(31 downto 0);
    m0_en_i       : in  std_ulogic;
    m0_burst_i    : in  unsigned(5 downto 0);
    m0_we_i       : in  std_ulogic;
    m0_dready_o   : out std_ulogic;
    m0_complete_o : out std_ulogic;
    m0_err_o      : out std_ulogic;

    -- Master 1: mem0 only
    m1_addr_i     : in  unsigned(31 downto 0);
    m1_din_i      : in  std_ulogic_vector(31 downto 0);
    m1_dout_o     : out std_ulogic_vector(31 downto 0);
    m1_en_i       : in  std_ulogic;
    m1_burst_i    : in  unsigned(5 downto 0);
    m1_we_i       : in  std_ulogic;
    m1_dready_o   : out std_ulogic;
    m1_complete_o : out std_ulogic;
    m1_err_o      : out std_ulogic;

    -- Master 2: mem0 + mem2
    m2_addr_i     : in  unsigned(31 downto 0);
    m2_din_i      : in  std_ulogic_vector(31 downto 0);
    m2_dout_o     : out std_ulogic_vector(31 downto 0);
    m2_en_i       : in  std_ulogic;
    m2_burst_i    : in  unsigned(5 downto 0);
    m2_we_i       : in  std_ulogic;
    m2_dready_o   : out std_ulogic;
    m2_complete_o : out std_ulogic;
    m2_err_o      : out std_ulogic;
    -- Slave 0: driven by a Python WbMemoryModel(prefix="s0_")
    s0_m_dat_i   : in  std_ulogic_vector(31 downto 0);
    s0_m_ack_i   : in  std_ulogic;
    s0_m_err_i   : in  std_ulogic;
    s0_m_stall_i : in  std_ulogic;
    s0_m_adr_o   : out unsigned(31 downto 0);
    s0_m_dat_o   : out std_ulogic_vector(31 downto 0);
    s0_m_we_o    : out std_ulogic;
    s0_m_stb_o   : out std_ulogic;
    s0_m_cyc_o   : out std_ulogic;
    -- Slave 1: driven by a Python WbMemoryModel(prefix="s1_")
    s1_m_dat_i   : in  std_ulogic_vector(31 downto 0);
    s1_m_ack_i   : in  std_ulogic;
    s1_m_err_i   : in  std_ulogic;
    s1_m_stall_i : in  std_ulogic;
    s1_m_adr_o   : out unsigned(31 downto 0);
    s1_m_dat_o   : out std_ulogic_vector(31 downto 0);
    s1_m_we_o    : out std_ulogic;
    s1_m_stb_o   : out std_ulogic;
    s1_m_cyc_o   : out std_ulogic;
    -- Slave 2: driven by a Python WbMemoryModel(prefix="s2_")
    s2_m_dat_i   : in  std_ulogic_vector(31 downto 0);
    s2_m_ack_i   : in  std_ulogic;
    s2_m_err_i   : in  std_ulogic;
    s2_m_stall_i : in  std_ulogic;
    s2_m_adr_o   : out unsigned(31 downto 0);
    s2_m_dat_o   : out std_ulogic_vector(31 downto 0);
    s2_m_we_o    : out std_ulogic;
    s2_m_stb_o   : out std_ulogic;
    s2_m_cyc_o   : out std_ulogic
  );
end entity wb_ixs_wrapper;

architecture rtl of wb_ixs_wrapper is

  constant MasterConfig : wb_master_config_array_t := (
    wb_master("slv1 mem0 mem2"),
    wb_master("mem0"),
    wb_master("mem0 mem2"));

  constant SlaveMap : wb_slave_config_array_t := (
    wb_slave("slv1", 0,   10),
    wb_slave("mem0", 128, 10),
    wb_slave("mem2", 256, 10));

  signal m0_wb_in  : wb_master_ifc_in_t;
  signal m0_wb_out : wb_master_ifc_out_t;
  signal m1_wb_in  : wb_master_ifc_in_t;
  signal m1_wb_out : wb_master_ifc_out_t;
  signal m2_wb_in  : wb_master_ifc_in_t;
  signal m2_wb_out : wb_master_ifc_out_t;

  signal ixs_min_i  : wb_slave_ifc_in_array_t(2 downto 0);
  signal ixs_min_o  : wb_slave_ifc_out_array_t(2 downto 0);
  signal ixs_sout_i : wb_master_ifc_in_array_t(2 downto 0);
  signal ixs_sout_o : wb_master_ifc_out_array_t(2 downto 0);


begin

  M0 : entity work.wb_master
    port map (
      clk_i => clk_i, reset_n_i => reset_n_i,
      addr_i => m0_addr_i, din_i => m0_din_i, dout_o => m0_dout_o,
      en_i => m0_en_i, burst_i => m0_burst_i, we_i => m0_we_i,
      dready_o => m0_dready_o, complete_o => m0_complete_o, err_o => m0_err_o,
      master_out_i => m0_wb_in, master_out_o => m0_wb_out);

  M1 : entity work.wb_master
    port map (
      clk_i => clk_i, reset_n_i => reset_n_i,
      addr_i => m1_addr_i, din_i => m1_din_i, dout_o => m1_dout_o,
      en_i => m1_en_i, burst_i => m1_burst_i, we_i => m1_we_i,
      dready_o => m1_dready_o, complete_o => m1_complete_o, err_o => m1_err_o,
      master_out_i => m1_wb_in, master_out_o => m1_wb_out);

  M2 : entity work.wb_master
    port map (
      clk_i => clk_i, reset_n_i => reset_n_i,
      addr_i => m2_addr_i, din_i => m2_din_i, dout_o => m2_dout_o,
      en_i => m2_en_i, burst_i => m2_burst_i, we_i => m2_we_i,
      dready_o => m2_dready_o, complete_o => m2_complete_o, err_o => m2_err_o,
      master_out_i => m2_wb_in, master_out_o => m2_wb_out);

  ixs_min_i(0) <= m0_wb_out;
  ixs_min_i(1) <= m1_wb_out;
  ixs_min_i(2) <= m2_wb_out;
  m0_wb_in <= ixs_min_o(0);
  m1_wb_in <= ixs_min_o(1);
  m2_wb_in <= ixs_min_o(2);

-- Slave side is plain wiring; the memories live in the test as
  -- WbMemoryModel instances, so any wait states are driven explicitly there.
  s0_m_adr_o <= ixs_sout_o(0).adr;
  s0_m_dat_o <= ixs_sout_o(0).dat;
  s0_m_we_o  <= ixs_sout_o(0).we;
  s0_m_stb_o <= ixs_sout_o(0).stb;
  s0_m_cyc_o <= ixs_sout_o(0).cyc;
  ixs_sout_i(0).dat   <= s0_m_dat_i;
  ixs_sout_i(0).ack   <= s0_m_ack_i;
  ixs_sout_i(0).err   <= s0_m_err_i;
  ixs_sout_i(0).stall <= s0_m_stall_i;
  s1_m_adr_o <= ixs_sout_o(1).adr;
  s1_m_dat_o <= ixs_sout_o(1).dat;
  s1_m_we_o  <= ixs_sout_o(1).we;
  s1_m_stb_o <= ixs_sout_o(1).stb;
  s1_m_cyc_o <= ixs_sout_o(1).cyc;
  ixs_sout_i(1).dat   <= s1_m_dat_i;
  ixs_sout_i(1).ack   <= s1_m_ack_i;
  ixs_sout_i(1).err   <= s1_m_err_i;
  ixs_sout_i(1).stall <= s1_m_stall_i;
  s2_m_adr_o <= ixs_sout_o(2).adr;
  s2_m_dat_o <= ixs_sout_o(2).dat;
  s2_m_we_o  <= ixs_sout_o(2).we;
  s2_m_stb_o <= ixs_sout_o(2).stb;
  s2_m_cyc_o <= ixs_sout_o(2).cyc;
  ixs_sout_i(2).dat   <= s2_m_dat_i;
  ixs_sout_i(2).ack   <= s2_m_ack_i;
  ixs_sout_i(2).err   <= s2_m_err_i;
  ixs_sout_i(2).stall <= s2_m_stall_i;

  DUT : entity work.wb_ixs
    generic map (MasterConfig => MasterConfig, SlaveMap => SlaveMap)
    port map (
      clk_i => clk_i, reset_n_i => reset_n_i,
      master_in_i => ixs_min_i, master_in_o => ixs_min_o,
      slave_out_i => ixs_sout_i, slave_out_o => ixs_sout_o);

end architecture rtl;
