-------------------------------------------------------------------------------
-- Title      : Testbench for design "wb_interconnect"
-- Project    : 
-------------------------------------------------------------------------------
-- File       : wb_interconnect_tb.vhd
-- Author     : malte  <malte@tp13>
-- Company    : 
-- Created    : 2018-04-29
-- Last update: 2019-01-20
-- Platform   : 
-- Standard   : VHDL'93/02
-------------------------------------------------------------------------------
-- Description: 
-------------------------------------------------------------------------------
-- Copyright (c) 2018 
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author  Description
-- 2018-04-29  1.0      malte	Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.textio.all;

use work.wb_test_p.all;
use work.wishbone_p.all;

entity wb_interconnect_tb is
end entity wb_interconnect_tb;

architecture behav of wb_interconnect_tb is

  -- component generics
  constant MasterConfig : wb_master_config_array_t := (
    wb_master("slv1 mem0 mem2"),
    wb_master("mem0"),
    wb_master("mem0 mem2"));
  
  constant SlaveMap     : wb_slave_config_array_t := (
    wb_slave("slv1",0,10),
    wb_slave("mem0",128,10),
    wb_slave("mem2",256,10)  
    );
      
  -- component ports
  signal clk      : std_ulogic := '0';
  signal reset_n  : std_ulogic;
  signal master_in_in  : wb_slave_ifc_in_array_t(MasterConfig'length-1 downto 0);
  signal master_in_out  : wb_slave_ifc_out_array_t(MasterConfig'length-1 downto 0);
  signal slaves_out_in : wb_master_ifc_in_array_t(SlaveMap'length-1 downto 0);
  signal slaves_out_out : wb_master_ifc_out_array_t(SlaveMap'length-1 downto 0);

  type master_in_array_t is array (natural range <>) of master_in_t;
  type master_out_array_t is array (natural range <>) of master_out_t;
  signal m_in : master_in_array_t(2 downto 0);
  signal m_out : master_out_array_t(2 downto 0);

  signal slave_in : wb_slave_ifc_in_array_t(2 downto 0);
  signal slave_out : wb_slave_ifc_out_array_t(2 downto 0);

  type slave_t is array (natural range <>) of std_ulogic_vector(31 downto 0);
  type slave_array_t is array (natural range <>) of slave_t(31 downto 0);
  signal slave_data : slave_array_t(2 downto 0) := ((others => X"22220000"),(others => X"11110000"),(others => X"00000000"));

begin  -- architecture behav

  -- component instantiation
  DUT: entity work.wb_ixs
    generic map (
      MasterConfig => MasterConfig,
      SlaveMap     => SlaveMap)
    port map (
      clk_i      => clk,
      reset_n_i  => reset_n,
      master_in_i  => master_in_in,
      master_in_o  => master_in_out,
      slave_out_i => slaves_out_in,
      slave_out_o => slaves_out_out);

  -- clock generation
  clk <= not clk after 10 ns;

  master: for i in 0 to 2 generate
    signal master_out_in : wb_master_ifc_in_t;
    signal master_out_out : wb_master_ifc_out_t;
    signal dout : std_ulogic_vector(31 downto 0);
  begin
    wb_master_1: entity work.wb_master
      port map (
        clk_i      => clk,
        reset_n_i  => reset_n,
        addr_i     => m_out(i).addr,
        din_i      => m_out(i).dout,
        dout_o     => m_in(i).din,
        en_i       => m_out(i).en,
        burst_i    => m_out(i).burst,
        we_i       => m_out(i).we,
        dready_o   => m_in(i).dready,
        complete_o => m_in(i).complete,
        err_o      => m_in(i).err,
        master_out_i => master_out_in,
        master_out_o => master_out_out);

    master_in_in(i) <= master_out_out;
    master_out_in <= master_in_out(i);
  end generate master;

  slave: for i in 0 to 2 generate
    process is
    begin  -- process
      slaves_out_in(i).dat <= (others => '0');
      slaves_out_in(i).ack <= '0';
      slaves_out_in(i).stall <= '1';
      slaves_out_in(i).err <= '0';
      wait until rising_edge(clk) and slaves_out_out(i).cyc = '1' and slaves_out_out(i).stb = '1';
      slaves_out_in(i).ack <= '1';
      slaves_out_in(i).stall <= '0';
      if slaves_out_out(i).we = '1' then
        slave_data(i)(to_integer(slaves_out_out(i).adr))(15 downto 0) <= slaves_out_out(i).dat(15 downto 0);
      else
        slaves_out_in(i).dat(15 downto 0) <= slave_data(i)(to_integer(slaves_out_out(i).adr))(15 downto 0);
      end if;
      wait until rising_edge(clk);      
    end process;
  end generate slave;

  process
    variable result : std_ulogic_vector(31 downto 0);
    variable data1 : data_array_t(7 downto 0);
    variable data2 : data_array_t(7 downto 0);
  begin
    reset_n <= '0';
    wait for 33 ns;

    reset_n <= '1';

    wait until rising_edge(clk);

    -- test that slave boundaries are correct 
    master_access(0,X"0000BEEF",result,'1',m_in(0),m_out(0));
    master_access(9,X"0000BEE0",result,'1',m_in(0),m_out(0));
    assert slave_data(0)(0) = X"0000BEEF" and slave_data(0)(9) = X"0000BEE0" report "slave 0 boundaries not correct" severity error;

    master_access(128,X"0000AA99",result,'1',m_in(0),m_out(0));
    master_access(137,X"0000BB77",result,'1',m_in(0),m_out(0));
    assert slave_data(1)(0) = X"1111AA99" and slave_data(1)(9) = X"1111BB77" report "slave 1 boundaries not correct" severity error;

    master_access(256,X"0000DDAB",result,'1',m_in(0),m_out(0));
    master_access(265,X"0000BE88",result,'1',m_in(0),m_out(0));
    assert slave_data(2)(0) = X"2222DDAB" and slave_data(2)(9) = X"2222BE88" report "slave 2 boundaries not correct" severity error;

    -- test that slaves can be accessed by masters
    master_access(1,X"0000ABCD",result,'1',m_in(0),m_out(0));
    master_access(129,X"0000DABC",result,'1',m_in(0),m_out(0));
    master_access(257,X"0000CDAB",result,'1',m_in(0),m_out(0));
    assert slave_data(0)(1) = X"0000ABCD" and
      slave_data(1)(1) = X"1111DABC" and
      slave_data(2)(1) = X"2222CDAB"
      report "master 0 slave access not correct" severity error;

    master_access(2,X"0000ABCD",result,'1',m_in(1),m_out(1));
    master_access(130,X"0000DABC",result,'1',m_in(1),m_out(1));
    master_access(258,X"0000CDAB",result,'1',m_in(1),m_out(1));
    assert slave_data(0)(2) = X"00000000" and
      slave_data(1)(2) = X"1111DABC" and
      slave_data(2)(2) = X"22220000"
      report "master 1 slave access not correct" severity error;

    master_access(3,X"0000ABCD",result,'1',m_in(2),m_out(2));
    master_access(131,X"0000DABC",result,'1',m_in(2),m_out(2));
    master_access(259,X"0000CDAB",result,'1',m_in(2),m_out(2));
    assert slave_data(0)(3) = X"00000000" and
      slave_data(1)(3) = X"1111DABC" and
      slave_data(2)(3) = X"2222CDAB"
      report "master 2 slave access not correct" severity error;
    

    -- test that data is read correctly
    master_access(129,X"00000000",result,'0',m_in(0),m_out(0));
    assert result = X"0000DABC" report "read slave 0 at addr 129 returns wrong data" severity error;

    master_access(130,X"00000000",result,'0',m_in(1),m_out(1));
    assert result = X"0000DABC" report "read slave 1 at addr 130 returns wrong data" severity error;
    
    master_access(131,X"00000000",result,'0',m_in(2),m_out(2));
    assert result = X"0000DABC" report "read slave 2 at addr 131 returns wrong data" severity error;
    

    -- multi master access; should be handled in parallel
    wait until rising_edge(clk);
    wait for 1 ps;
    m_out(0).addr <= to_unsigned(4,32);
    m_out(0).en <= '1';
    m_out(0).we <= '1';
    m_out(0).dout <= X"0000B123";

    m_out(1).addr <= to_unsigned(132,32);
    m_out(1).en <= '1';
    m_out(1).we <= '1';
    m_out(1).dout <= X"0000C123";

    m_out(2).addr <= to_unsigned(260,32);
    m_out(2).en <= '1';
    m_out(2).we <= '1';
    m_out(2).dout <= X"0000D123";

    wait until rising_edge(clk);
    wait for 1 ps;
    m_out(0).en <= '0';
    m_out(1).en <= '0';
    m_out(2).en <= '0';
    
    wait until rising_edge(clk);
    wait for 1 ps;

    assert slave_data(0)(4) = X"0000B123" and
      slave_data(1)(4) = X"1111C123" and
      slave_data(2)(4) = X"2222D123"
      report "parallel multi master access does not work correctly" severity error;

    
    -- multi master access; must be handled sequential
    wait until rising_edge(clk);
    wait for 1 ps;
    m_out(0).addr <= to_unsigned(133,32);
    m_out(0).en <= '1';
    m_out(0).we <= '1';
    m_out(0).dout <= X"0000B999";

    m_out(1).addr <= to_unsigned(134,32);
    m_out(1).en <= '1';
    m_out(1).we <= '1';
    m_out(1).dout <= X"0000C999";

    m_out(2).addr <= to_unsigned(135,32);
    m_out(2).en <= '1';
    m_out(2).we <= '1';
    m_out(2).dout <= X"0000D999";

    wait until rising_edge(clk);
    wait for 1 ps;

    wait until rising_edge(clk);
    wait for 1 ps;
    
    wait until rising_edge(clk);
    wait for 1 ps;

    assert slave_data(1)(5) = X"1111B999" report "sequential multi master access: expected master 0 to be granted access first" severity error;

    wait until rising_edge(clk);
    wait for 1 ps;
    wait until rising_edge(clk);
    wait for 1 ps;

    assert slave_data(1)(6) = X"1111C999" report "sequential multi master access: expected master 1 to be granted access second" severity error;

    wait until rising_edge(clk);
    wait for 1 ps;
    wait until rising_edge(clk);
    wait for 1 ps;

    assert slave_data(1)(7) = X"1111D999" report "sequential multi master access: expected master 2 to be granted access last" severity error;

    m_out(0).en <= '0';
    m_out(1).en <= '0';
    m_out(2).en <= '0';

    wait for 100 ns; -- wait until memory access is completed

    -- burst test
    data1 := (X"00000000",X"00000001",X"00000002",X"00000003",X"00000004",X"00000005",X"00000006",X"00000007");
    master_access_burst(130,data1,data2,'1',to_unsigned(7,3),m_in(0),m_out(0));
    master_access_burst(128,data1,data2,'0',to_unsigned(7,3),m_in(0),m_out(0));

    assert data2 = (X"00000002",X"00000003",X"00000004",X"00000005",X"00000006",X"00000007",X"00000000",X"00000001") report "burst read failed" severity error;

    write(output,"all tests complete" & LF);

    wait;
  end process;

  

end architecture behav;
