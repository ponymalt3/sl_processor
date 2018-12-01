-------------------------------------------------------------------------------
-- Title      : Testbench for design "wb_cache"
-- Project    : 
-------------------------------------------------------------------------------
-- File       : wb_cache_tb.vhd
-- Author     : malte  <malte@tp13>
-- Company    : 
-- Created    : 2018-11-22
-- Last update: 2018-12-01
-- Platform   : 
-- Standard   : VHDL'93/02
-------------------------------------------------------------------------------
-- Description: 
-------------------------------------------------------------------------------
-- Copyright (c) 2018 
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author  Description
-- 2018-11-22  1.0      malte	Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;

entity wb_cache_tb is
end entity wb_cache_tb;

architecture behav of wb_cache_tb is

  type cache_ifc_t is record
    clk      : std_logic;
    addr     : unsigned(31 downto 0);
    din      : std_ulogic_vector(31 downto 0);
    dout     : std_logic_vector(31 downto 0);
    en       : std_ulogic;
    we       : std_ulogic;
    complete : std_logic;
    inv_addr : unsigned(31 downto 0);
    inv_en : std_ulogic;
  end record cache_ifc_t;

  procedure cache_write (
    addr             : in    natural;
    din              : in    std_ulogic_vector(31 downto 0);
    signal cache_ifc : inout cache_ifc_t) is
  begin  -- procedure cache_write

    cache_ifc.addr <= to_unsigned(addr,32);
    cache_ifc.din <= din;
    cache_ifc.en <= '1';
    cache_ifc.we <= '1';

    wait until rising_edge(cache_ifc.clk) and cache_ifc.complete = '1';
    wait for 1 ps;

    cache_ifc.en <= '0';
    cache_ifc.we <= '0';
    
  end procedure cache_write;

  procedure cache_read (
    addr             : in    natural;
    dout             : out   std_ulogic_vector(31 downto 0);
    signal cache_ifc : inout cache_ifc_t) is
  begin  -- procedure cache_read

    cache_ifc.addr <= to_unsigned(addr,32);
    cache_ifc.din <= (others => '0');
    cache_ifc.en <= '1';
    cache_ifc.we <= '0';

    wait until rising_edge(cache_ifc.clk) and cache_ifc.complete = '1';
    wait for 1 ps;
 
    cache_ifc.en <= '0';
    cache_ifc.we <= '0';

    dout := to_stdUlogicVector(cache_ifc.dout);
    
  end procedure cache_read;

  procedure invalidate_addr (
    addr             : in natural;
    signal cache_ifc : inout cache_ifc_t) is
    variable rdata : std_ulogic_vector(31 downto 0);
  begin  -- procedure invalidate_addr
    wait until rising_edge(cache_ifc.clk);
    cache_ifc.inv_addr <= to_unsigned(addr,32);
    cache_ifc.inv_en <= '1';
    wait until rising_edge(cache_ifc.clk);
    cache_ifc.en <= '0';
  end procedure invalidate_addr;

  procedure cache_flush_by_reload_line (
    addr             : in natural;
    signal cache_ifc : inout cache_ifc_t) is
    variable rdata : std_ulogic_vector(31 downto 0);
  begin  -- procedure cache_flush_by_reload_line
    --wait until rising_edge(cache_ifc.clk);
    cache_read((addr+64) mod 256,rdata,cache_ifc);
    wait for 200 ns;
  end procedure cache_flush_by_reload_line;

  -- component generics
  constant WordsPerLine  : natural := 8;
  constant NumberOfLines : natural := 48;
  constant WriteTrough   : boolean := false;

  -- component ports
  signal clk            : std_ulogic := '0';
  signal reset_n        : std_ulogic;
  signal addr           : unsigned(31 downto 0);
  signal din            : std_ulogic_vector(31 downto 0);
  signal dout           : std_ulogic_vector(31 downto 0);
  signal en             : std_ulogic;
  signal we             : std_ulogic;
  signal complete       : std_ulogic;
  signal err            : std_ulogic;
  signal snooping_addr  : unsigned(31 downto 0);
  signal snooping_en    : std_ulogic;
  signal master_out_in  : wb_master_ifc_in_t;
  signal master_out_out : wb_master_ifc_out_t;

  signal cache0 : cache_ifc_t;

  type mem_t is array (natural range <>) of std_ulogic_vector(31 downto 0);
  signal mem0 : mem_t(511 downto 0) := (others => (others => '1'));

begin  -- architecture behav

  -- component instantiation
  DUT: entity work.wb_cache
    generic map (
      WordsPerLine  => 4,
      NumberOfLines => 16,
      WriteTrough   => false)
    port map (
      clk_i           => clk,
      reset_n_i       => reset_n,
      addr_i          => cache0.addr,
      din_i           => cache0.din,
      dout_o          => dout,
      en_i            => cache0.en,
      we_i            => cache0.we,
      complete_o      => complete,
      err_o           => open,
      snooping_addr_i => snooping_addr,
      snooping_en_i   => snooping_en,
      master_out_i    => master_out_in,
      master_out_o    => master_out_out);

  process is
  begin  -- process
    master_out_in.dat <= (others => '0');
    master_out_in.ack <= '0';
    master_out_in.stall <= '0';
    master_out_in.err <= '0';
    wait until rising_edge(clk) and master_out_out.cyc = '1' and master_out_out.stb = '1';
    master_out_in.ack <= '1';
    if master_out_out.we = '1' then
      mem0(to_integer(master_out_out.adr)) <= master_out_out.dat;
    else
      master_out_in.dat <= mem0(to_integer(master_out_out.adr));
    end if;
    wait until rising_edge(clk);      
  end process;

  -- clock generation
  clk <= not clk after 10 ns;

  cache0.clk <= clk;
  cache0.dout <= std_logic_vector(dout);
  cache0.complete <= complete;

  process
    variable rdata : std_ulogic_vector(31 downto 0);
    variable wdata : std_ulogic_vector(31 downto 0);
    variable startTime : time;
  begin
    reset_n <= '0';

    cache0 <= ('Z',to_unsigned(0,32),(others => '0'),(others => 'Z'),'0','0','Z',to_unsigned(0,32),'0');

    snooping_addr <= to_unsigned(0,32);
    snooping_en <= '0';

    wait for 33.2 ns;

    reset_n <= '1';

    wait for 500 ns;


    -- test that simple read with fetch works
    cache_read(256,rdata,cache0);
    assert rdata = X"FFFFFFFF" report "test that simple read with fetch works failed" severity error;

    wait for 1us;

    -- test that cached write to existing line works
    wdata := X"ABCDEF00";
    cache_write(257,wdata,cache0);
    -- force write back of cache line
    cache_flush_by_reload_line(256,cache0);
    assert mem0(257) = X"ABCDEF00" report "test that cached write to existing line works: cache line not written back" severity error;
    
    cache_read(257,rdata,cache0);
    assert now-startTime < 40 ns report "test that cached write to existing line works: cache line not available" severity error;
    assert rdata = X"ABCDEF00" report "test that cached write to existing line works: cache line not updated" severity error;

    wait for 1us;
    
    -- test that write to invalid cache line works
    wdata := X"ABABABAB";
    startTime := now;
    cache_write(27,wdata,cache0);
    cache_read(27,rdata,cache0);
    assert (now-startTime) < 400 ns
      report "test that write to invalid cache line works: cache fetch line takes too much time" severity warning;
    assert mem0(27) = X"FFFFFFFF"
      report "test that write to invalid cache line works: data should not written back yet" severity error;
    assert rdata = X"ABABABAB"
      report "test that write to invalid cache line works: cache line not updated" severity error;
    cache_flush_by_reload_line(27,cache0);
    assert mem0(27) = X"ABABABAB"
      report "test that write to invalid cache line works: data should be written back" severity error;

    wait for 1us;

    -- test that write to existing dirty cache line works
    wdata := X"CCCCCCAB";
    startTime := now;
    cache_write(26,wdata,cache0); -- make dirty
    wdata := X"DEADBEEF";
    cache_write(90,wdata,cache0);
    cache_read(90,rdata,cache0);
    assert (now-startTime) < 400 ns
      report "test that write to existing dirty cache line works: cache fetch line takes too much time" severity warning;
    assert mem0(26) = X"CCCCCCAB"
      report "test that write to existing dirty cache line works: dirty line should be written back already" severity error;
    assert rdata = X"DEADBEEF"
      report "test that write to existing dirty cache line works: cache line not updated" severity error;
    cache_flush_by_reload_line(90,cache0);
    assert mem0(90) = X"DEADBEEF"
      report "test that write to existing dirty cache line works: data should be written back" severity error;

    wait for 1us;

     -- test that write is delayed correctly
    wdata := X"1234BEAD";
    cache_write(89,wdata,cache0); -- make dirty
    wdata := X"DEADBE3F";
    cache_write(24,wdata,cache0);
    wdata := X"DEADB33F";
    cache_write(27,wdata,cache0);
    cache_read(27,rdata,cache0);
    assert mem0(89) = X"1234BEAD"
      report "test that write is delayed correctly: dirty line should be written back already" severity error;
    assert rdata = X"DEADB33F"
      report "test that write is delayed correctly: cache line not updated" severity error;
    cache_flush_by_reload_line(27,cache0);
    assert mem0(24) = X"DEADBE3F" and mem0(27) = X"DEADB33F"
      report "test that write is delayed correctly: data should be written back" severity error;
    


    wait;
    
  end process;

  

end architecture behav;
