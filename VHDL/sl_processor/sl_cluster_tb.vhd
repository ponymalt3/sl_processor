-------------------------------------------------------------------------------
-- Title      : Testbench for design "sl_cluster"
-- Project    : 
-------------------------------------------------------------------------------
-- File       : sl_cluster_tb.vhd
-- Author     : malte  <malte@tp13>
-- Company    : 
-- Created    : 2018-06-03
-- Last update: 2018-12-31
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
    wait until falling_edge(clk);
    wait for 1 ps;
    data := master_in.dat;
    wait until rising_edge(clk);
    master_out.cyc <= '0';
  end procedure;

  type code_array_t is array (natural range <>) of std_ulogic_vector(15 downto 0);
  constant code : code_array_t := (
    X"b408",X"b000",X"a000",X"ffff",
    X"b200",X"b000",X"a000",X"ffff",
    X"b405",X"b000",X"a000",X"ffff",
    X"b405",X"b000",X"a000",X"ffff",
    X"b440",X"b200",X"f024",X"b2e0",
    X"c095",X"f085",X"b000",X"9020",
    X"b47c",X"b700",X"f140",X"b020",
    X"2020",X"9020",X"aff5",X"8023",
    X"9020",X"8007",X"a000",X"ffff",
    X"ffff",X"ffff",X"ffff",X"ffff",
    X"b440",X"b400",X"f024",X"b40c",
    X"b300",X"c095",X"f085",X"b000",
    X"9020",X"b47c",X"b700",X"f140",
    X"b020",X"2020",X"9020",X"aff5",
    X"8023",X"9020",X"8007",X"a000",
    X"ffff",X"ffff",X"ffff",X"ffff",
    X"b440",X"f024",X"b0e0",X"c095",
    X"f085",X"b000",X"9020",X"b47c",
    X"b700",X"f140",X"b020",X"2020",
    X"9020",X"aff5",X"8023",X"9020",
    X"8007",X"a000");

  constant ExtMemAddr : natural := 8*1024/4;

  -- component ports
  signal clk          : std_ulogic := '1';
  signal reset_n      : std_ulogic;
  signal core_en      : std_ulogic_vector(3 downto 0);
  signal core_reset_n : std_ulogic_vector(3 downto 0);
  signal wbs_in     : wb_slave_ifc_in_t;
  signal wbs_out    : wb_slave_ifc_out_t;

  signal ext_master_in  : wb_master_ifc_in_t;
  signal ext_master_out : wb_master_ifc_out_t;
  signal ext_master_out2 : wb_master_ifc_out_t;
  signal code_master_in  : wb_master_ifc_in_t;
  signal code_master_out : wb_master_ifc_out_t;
  signal master_in : wb_master_ifc_in_array_t(1 downto 0);
  signal master_out : wb_master_ifc_out_array_t(1 downto 0);
  signal slave_in : wb_slave_ifc_in_array_t(0 downto 0);
  signal slave_out : wb_slave_ifc_out_array_t(0 downto 0);

  signal mem_clk : std_ulogic;

begin  -- architecture behav

  mem_clk <= not clk;

  -- component instantiation
  DUT: entity work.sl_cluster
    generic map (
      LocalMemSizeInKB  => 2,
      ExtMemSizeInKB    => 8,
      CodeMemSizeInKB   => 8)
    port map (
      clk_i          => clk,
		mem_clk_i      => mem_clk,
      reset_n_i      => reset_n,
      core_en_i      => core_en,
      core_reset_n_i => core_reset_n,
      ext_master_i   => ext_master_in,
      ext_master_o   => ext_master_out,
      code_master_i  => code_master_in,
      code_master_o  => code_master_out);

  wb_ixs_1: entity work.wb_ixs
    generic map (
      MasterConfig => (
        wb_master("mem"),
        wb_master("mem")),
      SlaveMap     => (
      (0 => wb_slave("mem",0,4096))))
      
    port map (
      clk_i       => clk,
      reset_n_i   => reset_n,
      master_in_i => master_out,
      master_in_o => master_in,
      slave_out_i => slave_out,
      slave_out_o => slave_in);

    master_out(0) <= wbs_in;
    wbs_out <= master_in(0);
    master_out(1) <= code_master_out;
    code_master_in <= master_in(1);

  wb_mem_1: entity work.wb_mem
    generic map (
      MemSizeInKB => 16)
    port map (
      clk_i     => clk,
      mem_clk_i => mem_clk,
      reset_n_i => reset_n,
      slave0_i   => ext_master_out2,
      slave0_o   => ext_master_in,
      slave1_i   => slave_in(0),
      slave1_o   => slave_out(0));

  ext_master_out2 <=
    (ext_master_out.adr+to_unsigned(2048,32),
     ext_master_out.dat,
     ext_master_out.we,
     ext_master_out.sel,
     ext_master_out.stb,
     ext_master_out.cyc);

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
      bus_write(clk,wbs_out,wbs_in,i,X"0000" & code(i));
    end loop;  -- i

    -- init ext data
    bus_write(clk,wbs_out,wbs_in,ExtMemAddr+0,X"01000000");
    bus_write(clk,wbs_out,wbs_in,ExtMemAddr+2,X"02000000");
    bus_write(clk,wbs_out,wbs_in,ExtMemAddr+4,X"03000000");
    bus_write(clk,wbs_out,wbs_in,ExtMemAddr+6,X"04000000");

    wait for 100 ns;

    reset_n <= '0';

    wait for 23 ns;

    reset_n <= '1';

    wait for 1 us; -- wait to initialize cache

    core_reset_n <= "0000";

    wait until rising_edge(clk);
    wait until rising_edge(clk);

    core_reset_n <= "1111";

    wait for 75 ns;

    core_en <= "1111";

    wait for 2 us;

    bus_read(clk,wbs_out,wbs_in,ExtMemAddr+1,result);
    assert result = X"07000000" report "expect qfp value 7 at ext addr 1" severity error;
    bus_read(clk,wbs_out,wbs_in,ExtMemAddr+3,result);
    assert result = X"202E0000" report "expect qfp value 46 at ext addr 3" severity error;
    bus_read(clk,wbs_out,wbs_in,ExtMemAddr+5,result);
    assert result = X"21290000" report "expect qfp value 297 at ext addr 5" severity error;
    bus_read(clk,wbs_out,wbs_in,ExtMemAddr+6,result);
    assert result = X"04000000" report "expect qfp value 4 at ext addr 6" severity error;

    write(output,"all tests complete" & LF);

    wait;
  end process;

end architecture behav;
