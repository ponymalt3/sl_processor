-------------------------------------------------------------------------------
-- Title      : Testbench for design "sl_processor"
-- Project    : 
-------------------------------------------------------------------------------
-- File       : sl_processor_tb.vhd
-- Author     : malte  <malte@tp13>
-- Company    : 
-- Created    : 2018-04-02
-- Last update: 2018-04-02
-- Platform   : 
-- Standard   : VHDL'93/02
-------------------------------------------------------------------------------
-- Description: 
-------------------------------------------------------------------------------
-- Copyright (c) 2018 
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author  Description
-- 2018-04-02  1.0      malte	Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity sl_processor_tb is
end entity sl_processor_tb;

architecture behav of sl_processor_tb is

  type code_mem_t is array (natural range <>) of std_ulogic_vector(15 downto 0);

  constant code : code_mem_t := (X"BB10",X"9028",X"FFFF",X"FFFF",X"FFFF");

  signal clk       : std_ulogic := '0';
  signal reset_n   : std_ulogic;
  signal code_addr : unsigned(15 downto 0);
  signal code_data : std_ulogic_vector(15 downto 0);

  signal ext_mem_addr  : unsigned(31 downto 0);
  signal ext_mem_dout  : std_ulogic_vector(31 downto 0);
  signal ext_mem_din   : std_ulogic_vector(31 downto 0);
  signal ext_mem_rw    : std_ulogic;
  signal ext_mem_en    : std_ulogic;
  signal ext_mem_stall : std_ulogic;

begin  -- architecture behav

  code_data <= code(to_integer(unsigned(code_addr)));

  -- component instantiation
  DUT: entity work.sl_processor
    port map (
      clk_i       => clk,
      reset_n_i   => reset_n,
      reset_core_n_i => reset_n,
      code_addr_o => code_addr,
      code_data_i => code_data,
      ext_mem_addr_o  => ext_mem_addr,
      ext_mem_dout_o  => ext_mem_dout,
      ext_mem_din_i   => ext_mem_din,
      ext_mem_rw_o    => ext_mem_rw,
      ext_mem_en_o    => ext_mem_en,
      ext_mem_stall_i => ext_mem_stall,
      mem_addr_i      => to_unsigned(0,16),
      mem_din_i       => (others => '0'),
      mem_dout_o      => open,
      mem_we_i        => '0',
      mem_complete_o  => open,
      executed_addr_o => open);

  clk <= not clk after 10 ns;

  process
  begin
    reset_n <= '0';

    wait for 33 ns;

    reset_n <= '1';

    wait;

  end process;

  

end architecture behav;
