-------------------------------------------------------------------------------
-- Title      : Testbench for design "sl_cluster"
-- Project    : 
-------------------------------------------------------------------------------
-- File       : sl_cluster_tb.vhd
-- Author     : malte  <malte@tp13>
-- Company    : 
-- Created    : 2018-06-03
-- Last update: 2018-08-11
-- Platform   : 
-- Standard   : VHDL'93/02
-------------------------------------------------------------------------------
-- Description: 
-------------------------------------------------------------------------------
-- Copyright (c) 2018 
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author  Description
-- 2018-06-03  1.0      malte	Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.textio.all;

use work.wishbone_p.all;

entity sl_cluster_tb is
end entity sl_cluster_tb;

architecture behav of sl_cluster_tb is

  procedure bus_write (
    signal clk : in std_ulogic;
    signal master_in : in wb_master_ifc_in_t;
    signal master_out : out wb_master_ifc_out_t;
    addr : in natural;
    data : in std_ulogic_vector(31 downto 0)) is
  begin
    master_out.stb <= '0';
    master_out.cyc <= '0';
    master_out.we <= '0';
    master_out.sel <= "1111";
    wait until rising_edge(clk);
    master_out.stb <= '1';
    master_out.cyc <= '1';
    master_out.we <= '1';
    master_out.adr <= to_unsigned(addr,32);
    master_out.dat <= data;
    wait until rising_edge(clk) and master_in.stall = '0';
    master_out.stb <= '0';
    master_out.we <= '0';
    if master_in.ack /= '1' then
      wait until master_in.ack = '1';
    end if;
    wait until rising_edge(clk);
    master_out.cyc <= '0';
    wait for 1 ns;
  end procedure;

  procedure bus_read (
    signal clk : in std_ulogic;
    signal master_in : in wb_master_ifc_in_t;
    signal master_out : out wb_master_ifc_out_t;
    addr : in natural;
    data : out std_ulogic_vector(31 downto 0)) is
  begin
    master_out.stb <= '0';
    master_out.cyc <= '0';
    master_out.we <= '0';
    master_out.sel <= "1111";
    wait until rising_edge(clk);
    master_out.stb <= '1';
    master_out.cyc <= '1';
    master_out.adr <= to_unsigned(addr,32);
    wait until rising_edge(clk) and master_in.stall = '0';
    master_out.stb <= '0';
    if master_in.ack /= '1' then
      wait until master_in.ack = '1';
    end if;
    wait for 1 ps;
    data := master_in.dat;
    wait until rising_edge(clk);
    master_out.cyc <= '0';
  end procedure;

  type code_array_t is array (natural range <>) of std_ulogic_vector(15 downto 0);
  constant code : code_array_t := (
    X"b440",
    X"9020",
    X"b040",
    X"4011",
    X"2021",
    X"f024",
    X"8013",
    X"f085",
    X"b40c",
    X"b300",
    X"c055",
    X"f085",
    X"b000",
    X"9010",
    X"b47c",
    X"b700",
    X"f140",
    X"b020",
    X"2011",
    X"9010",
    X"aff5",
    X"8013",
    X"9010");

  -- component ports
  signal clk          : std_ulogic := '1';
  signal reset_n      : std_ulogic;
  signal core_en      : std_ulogic_vector(3 downto 0);
  signal core_reset_n : std_ulogic_vector(3 downto 0);
  signal proc_wbm_in    : wb_master_ifc_in_t;
  signal proc_wbm_out   : wb_master_ifc_out_t;
  signal proc_wbs_in     : wb_slave_ifc_in_t;
  signal proc_wbs_out    : wb_slave_ifc_out_t;

begin  -- architecture behav

  -- component instantiation
  DUT: entity work.sl_cluster
    generic map (
      LocalMemSizeInKB  => 2,
      SharedMemSizeInKB => 8,
      CodeMemSizeInKB   => 8)
    port map (
      clk_i          => clk,
      reset_n_i      => reset_n,
      core_en_i      => core_en,
      core_reset_n_i => core_reset_n,
      master_i       => proc_wbm_in,
      master_o       => proc_wbm_out,
      slave_i        => proc_wbs_in,
      slave_o        => proc_wbs_out);

  clk <= not clk after 10 ns;

  process
    variable result : std_ulogic_vector(31 downto 0);
  begin
    reset_n <= '0';
    core_en <= (others => '0');
    core_reset_n <= (others => '1');

    wait for 33 ns;

    wait until rising_edge(clk);
    reset_n <= '1';

    -- load code
    for i in 0 to code'length-1 loop
      bus_write(clk,proc_wbs_out,proc_wbs_in,80*1024/4+i,X"0000" & code(i));
    end loop;  -- i

    -- init proc ids
    bus_write(clk,proc_wbs_out,proc_wbs_in,8*1024/4+4,X"01000000");
    bus_write(clk,proc_wbs_out,proc_wbs_in,10*1024/4+4,X"02000000");
    bus_write(clk,proc_wbs_out,proc_wbs_in,12*1024/4+4,X"03000000");
    bus_write(clk,proc_wbs_out,proc_wbs_in,14*1024/4+4,X"04000000");

    wait for 100 ns;

    core_reset_n <= "0000";

    wait until rising_edge(clk);
    wait until rising_edge(clk);

    core_reset_n <= "1111";

    wait for 75 ns;

    core_en <= "1111";

    wait for 1 us;

    bus_read(clk,proc_wbs_out,proc_wbs_in,2,result);
    assert result = X"01000000" report "expect qfp value 2 at addr 1" severity error;
    bus_read(clk,proc_wbs_out,proc_wbs_in,4,result);
    assert result = X"02000000" report "expect qfp value 4 at addr 2" severity error;
    bus_read(clk,proc_wbs_out,proc_wbs_in,6,result);
    assert result = X"03000000" report "expect qfp value 6 at addr 3" severity error;
    bus_read(clk,proc_wbs_out,proc_wbs_in,8,result);
    assert result = X"04000000" report "expect qfp value 8 at addr 4" severity error;

    write(output,"all tests complete" & LF);

    wait;
  end process;

    

    


  

end architecture behav;
