library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.textio.all;

use work.wishbone_p.all;

entity wb_interconnect is
  
  generic (
    MasterConfig : wb_master_config_array_t;
    SlaveMap     : wb_slave_config_array_t);

  port (
    clk_i     : in std_ulogic;
    reset_n_i : in std_ulogic;

    master_in_i : in wb_slave_ifc_in_array_t(MasterConfig'length-1 downto 0);
    master_in_o : out wb_slave_ifc_out_array_t(MasterConfig'length-1 downto 0);
    slave_out_i : in wb_master_ifc_in_array_t(SlaveMap'length-1 downto 0);
    slave_out_o : out wb_master_ifc_out_array_t(SlaveMap'length-1 downto 0));

end entity wb_interconnect;

architecture rtl of wb_interconnect is

  type wb_master_ifc_in_matrix_t is array (natural range <>) of wb_master_ifc_in_array_t(SlaveMap'length-1 downto 0);
  type wb_master_ifc_out_matrix_t is array (natural range <>) of wb_master_ifc_out_array_t(SlaveMap'length-1 downto 0);

  signal decode_ifc_in : wb_master_ifc_in_matrix_t(MasterConfig'length-1 downto 0);
  signal decode_ifc_out : wb_master_ifc_out_matrix_t(MasterConfig'length-1 downto 0);

begin  -- architecture rtl

  decode: for i in 0 to MasterConfig'length-1 generate
    constant CurSlaveMap : wb_slave_config_array_t := wb_interconnect_get_connected_slaves(MasterConfig(i),SlaveMap);
    wb_interconnect_decoder_1: entity work.wb_interconnect_decoder
      generic map (
        SlaveMap => CurSlaveMap)
      port map (
        clk_i        => clk_i,
        reset_n_i    => reset_n_i,
        master_in_i  => master_in_i(i),
        master_in_o  => master_in_o(i),
        master_out_i => decode_ifc_in(i)(CurSlaveMap'length-1 downto 0),
        master_out_o => decode_ifc_out(i)(CurSlaveMap'length-1 downto 0));
  end generate decode;

  arbiter: for i in 0 to SlaveMap'length-1 generate
    constant CurMasterConfig : wb_master_config_array_t := wb_interconnect_get_masters_for_slave(SlaveMap(i).name,SlaveMap,MasterConfig);
    signal m_in : wb_slave_ifc_in_array_t(CurMasterConfig'length-1 downto 0);
    signal m_out : wb_slave_ifc_out_array_t(CurMasterConfig'length-1 downto 0);
  begin
    
    arbiter_input: for j in 0 to CurMasterConfig'length-1 generate
      m_in(j) <= decode_ifc_out(CurMasterConfig(j).id)(CurMasterConfig(j).rel_slave_pos);
      decode_ifc_in(CurMasterConfig(j).id)(CurMasterConfig(j).rel_slave_pos) <= m_out(j);
    end generate arbiter_input;
    
    wb_interconnect_arbiter_1: entity work.wb_interconnect_arbiter
      generic map (
        MasterConfig => CurMasterConfig)
      port map (
        clk_i      => clk_i,
        reset_n_i  => reset_n_i,
        master_in_i  => m_in,
        master_in_o  => m_out,
        master_out_i => slave_out_i(i),
        master_out_o => slave_out_o(i));
    
  end generate arbiter;

end architecture rtl;
