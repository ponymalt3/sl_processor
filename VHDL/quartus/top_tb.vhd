-------------------------------------------------------------------------------
-- Title      : Testbench for design "top"
-- Project    : 
-------------------------------------------------------------------------------
-- File       : top_tb.vhd
-- Author     : malte  <malte@tp13>
-- Company    : 
-- Created    : 2018-07-06
-- Last update: 2018-07-24
-- Platform   : 
-- Standard   : VHDL'93/02
-------------------------------------------------------------------------------
-- Description: 
-------------------------------------------------------------------------------
-- Copyright (c) 2018 
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author  Description
-- 2018-07-06  1.0      malte	Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.textio.all;

entity top_tb is
end entity top_tb;

architecture behav of top_tb is

  type byte_array_t is array (natural range <>) of std_ulogic_vector(7 downto 0);

  procedure uart_read (
    signal rxd : in  std_ulogic;
    period     : in  time;
    data       : out std_ulogic_vector(7 downto 0)) is
    variable timeout : time := now;
    variable recv : std_ulogic_vector(7 downto 0);
  begin  -- procedure uart_read
    if rxd = '1' then
      wait until rxd = '0';
    end if;
    for i in 0 to 8 loop
      wait for period/2;
      if i = 0 then
        assert rxd = '0' report "expect start bit" severity error;
      else
        recv(i-1) := rxd;
      end if;
      wait for period/2;
    end loop;  -- i
    data := recv;
    -- write(output,"uart recv: " & integer'image(to_integer(unsigned(recv))) & LF);
  end procedure uart_read;

   procedure uart_read_array (
    signal rxd : in  std_ulogic;
    period     : in  time;
    data       : out byte_array_t) is
  begin  -- procedure uart_read
    for i in 0 to data'length-1 loop
      uart_read(rxd,period,data(i));
      wait for 100 ns;
    end loop;  -- i
  end procedure uart_read_array;

  procedure uart_write (
    signal txd : out std_ulogic;
    period     : in  time;
    data       : in  std_ulogic_vector(7 downto 0)) is
    variable buf : std_ulogic_vector(9 downto 0) := '1' & data & '0';
  begin  -- procedure uart_write
    for i in 0 to 9 loop
      txd <= buf(i);
      wait for period;
    end loop;  -- i   
  end procedure uart_write;

  procedure uart_write_array (
    signal txd : out std_ulogic;
    period : in time;
    data : in byte_array_t) is
  begin  -- procedure uart_write
    for i in 0 to data'length-1 loop
      uart_write(txd,period,data(i));
    end loop;  -- i
  end procedure uart_write_array;

  procedure uart_write16 (
    signal txd : out std_ulogic;
    period : in time;
    data16 : in unsigned(15 downto 0)) is
    variable data : std_ulogic_vector(15 downto 0) := to_stdULogicVector(std_logic_vector(data16));
  begin  -- procedure uart_write
    uart_write(txd,period,data(15 downto 8));
    uart_write(txd,period,data(7 downto 0));
  end procedure uart_write16;

  procedure uart_write24 (
    signal txd : out std_ulogic;
    period : in time;
    data24 : in unsigned(24 downto 0)) is
    variable data : std_ulogic_vector(23 downto 0) := to_stdULogicVector(std_logic_vector(data24));
  begin  -- procedure uart_write
    uart_write(txd,period,data(23 downto 16));
    uart_write(txd,period,data(15 downto 8));
    uart_write(txd,period,data(7 downto 0));
  end procedure uart_write24;

  procedure uart_write32 (
    signal txd : out std_ulogic;
    period : in time;
    data32 : in unsigned(31 downto 0)) is
    variable data : std_ulogic_vector(31 downto 0) := to_stdULogicVector(std_logic_vector(data32));
  begin  -- procedure uart_write
    uart_write(txd,period,data(31 downto 16));
    uart_write(txd,period,data(15 downto 0));
  end procedure uart_write32;

  -- component ports
  signal clock_50  : std_ulogic := '1';
  signal sw        : std_ulogic_vector(3 downto 0);
  signal key       : std_ulogic_vector(1 downto 0);
  signal led       : std_ulogic_vector(7 downto 0);
  signal gpio_0    : std_ulogic_vector(33 downto 0);

  alias reset_n : std_ulogic is key(0);
  alias core_txd : std_ulogic is gpio_0(23);
  alias core_rxd : std_ulogic is gpio_0(22);

begin  -- architecture behav

  -- component instantiation
  DUT: entity work.top
    port map (
      clock_50  => clock_50,
      sw        => sw,
      key       => key,
      led       => led,
      dram_addr => open,
      dram_dq   => open,
      gpio_0    => gpio_0,
      gpio_1    => open,
      gpio_2    => open);

  -- clock generation
  clock_50 <= not clock_50 after 10 ns;

  process
    variable buf1 : byte_array_t(31 downto 0);
    variable buf2 : byte_array_t(31 downto 0);
  begin
    reset_n <= '0';
    core_rxd <= '1';

    wait for 33 ns;

    reset_n <= '1';

    wait for 100 ns;

    -- test ping cmd
    uart_write(core_rxd,8.68us,X"04");
    uart_read(core_txd,8.68us,buf1(0));
    assert buf1(0) = X"04" report "ping cmd failed" severity error;

    -- test read/write cmd
    uart_write(core_rxd,8.68us,X"82"); -- write
    uart_write16(core_rxd,8.68us,to_unsigned(23,16)); -- addr
    uart_write16(core_rxd,8.68us,to_unsigned(4,16)); -- len
    buf1(15 downto 0) := (X"01",X"02",X"03",X"04",X"05",X"06",X"07",X"08",X"09",X"0A",X"0B",X"0C",X"0D",X"0E",X"0F",X"10");
    uart_write_array(core_rxd,8.68us,buf1(15 downto 0));
    uart_read(core_txd,8.68us,buf2(0));
    assert buf2(0) = X"82" report "write cmd failed" severity error;
    -- read back
    uart_write(core_rxd,8.68us,X"81"); -- read
    uart_write16(core_rxd,8.68us,to_unsigned(24,16)); -- addr
    uart_write16(core_rxd,8.68us,to_unsigned(3,16)); -- len
    uart_read_array(core_txd,8.68us,buf2(11 downto 0));
    uart_read(core_txd,8.68us,buf2(16));
    assert buf2(16) = X"81" report "read cmd failed" severity error;
    assert buf2(11 downto 0) = buf1(15 downto 4) report "read back data compare failed" severity error;
    -- second part
    uart_write(core_rxd,8.68us,X"81"); -- read
    uart_write16(core_rxd,8.68us,to_unsigned(23,16)); -- addr
    uart_write16(core_rxd,8.68us,to_unsigned(1,16)); -- len
    uart_read_array(core_txd,8.68us,buf2(3 downto 0));
    uart_read(core_txd,8.68us,buf2(16));
    assert buf2(16) = X"81" report "read cmd failed" severity error;
    assert buf2(3 downto 0) = buf1(3 downto 0) report "read back data compare for second block failed" severity error;

    -- test core cmd
    uart_write(core_rxd,8.68us,X"03"); -- core en
    uart_write(core_rxd,8.68us,X"FF"); -- reset
    uart_write(core_rxd,8.68us,X"FF"); -- en
    uart_read(core_txd,8.68us,buf2(0));
    assert buf2(0) = X"03" report "core cmd for reset failed" severity error;

    --uart_write(core_rxd,8.68us,X"03"); -- core en
    --uart_write(core_rxd,8.68us,X"00"); -- reset
    --uart_write(core_rxd,8.68us,X"FF"); -- en
    --uart_read(core_txd,8.68us,buf2(0));
    --assert buf2(0) = X"03" report "core cmd for enable failed" severity error;

    write(output,"all test complete" & LF);

    wait;
    
  end process;

end architecture behav;
