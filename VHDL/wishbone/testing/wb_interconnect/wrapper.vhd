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
    m2_err_o      : out std_ulogic
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

  type slave_mem_t is array (0 to 31) of std_ulogic_vector(31 downto 0);

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

  -- VHDL slave processes (matching original wb_interconnect_tb.vhd)
  slave_gen: for i in 0 to 2 generate
  begin
    process is
      variable slave_data : slave_mem_t := (others => (others => '0'));
      variable idx : natural;
    begin
      ixs_sout_i(i).dat   <= (others => '0');
      ixs_sout_i(i).ack   <= '0';
      ixs_sout_i(i).stall <= '1';
      ixs_sout_i(i).err   <= '0';
      wait until rising_edge(clk_i) and ixs_sout_o(i).cyc = '1' and ixs_sout_o(i).stb = '1';
      idx := to_integer(ixs_sout_o(i).adr(4 downto 0));
      ixs_sout_i(i).ack   <= '1';
      ixs_sout_i(i).stall <= '0';
      if ixs_sout_o(i).we = '1' then
        slave_data(idx) := ixs_sout_o(i).dat;
      else
        ixs_sout_i(i).dat <= slave_data(idx);
      end if;
      wait until rising_edge(clk_i);
    end process;
  end generate slave_gen;

  DUT : entity work.wb_ixs
    generic map (MasterConfig => MasterConfig, SlaveMap => SlaveMap)
    port map (
      clk_i => clk_i, reset_n_i => reset_n_i,
      master_in_i => ixs_min_i, master_in_o => ixs_min_o,
      slave_out_i => ixs_sout_i, slave_out_o => ixs_sout_o);

end architecture rtl;
