-------------------------------------------------------------------------------
-- Title      : Testbench for design "wb_cache"
-- Project    : 
-------------------------------------------------------------------------------
-- File       : wb_cache_tb.vhd
-- Author     : malte  <malte@tp13>
-- Company    : 
-- Created    : 2018-11-22
-- Last update: 2018-12-09
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
use ieee.math_real.all;

use IEEE.std_logic_1164.all;            -- basic logic types
use STD.textio.all;                     -- basic I/O
use IEEE.std_logic_textio.all;          -- I/O for logic types

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

  type mem_t is array (natural range <>) of std_ulogic_vector(31 downto 0);

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
    cache_ifc.inv_en <= '0';
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

  procedure random (
    variable seed   : inout unsigned(63 downto 0);
    constant modulo : in    positive;
    variable rand   : out   natural) is
    variable seed1 : positive := to_integer(seed(31 downto 0) and X"3FFFFFFF")+1;
    variable seed2 : positive := to_integer(seed(63 downto 32) and X"3FFFFFFF")+1;
    variable r : real;
  begin  -- procedure random
    uniform(seed1,seed2,r);
    seed := to_unsigned(seed2,32) & to_unsigned(seed1,32);
    rand := integer(trunc(real(modulo)*r)); 
  end procedure random;

  procedure cache_access_test (
    reads       : in natural;
    writes      : in natural;
    invalidates : in natural;
    memsize     : in natural;
    signal cache_ifc : inout cache_ifc_t) is
    variable rdata : std_ulogic_vector(31 downto 0);
    variable mem : mem_t(memsize-1 downto 0) := (others => X"DEADBEEF");
    variable wdata : std_ulogic_vector(31 downto 0) := to_stdUlogicVector(std_logic_vector(to_unsigned(4326,32)));
    variable r : natural := reads;
    variable w : natural := writes;
    variable inv : natural := invalidates;
    variable inv_now : boolean := false;
    variable addr : natural;    
    variable seed : unsigned(63 downto 0) := to_unsigned(67492340,32) & to_unsigned(95432563,32);
    variable rand : natural;
    
    file output : text open write_mode is "STD_OUTPUT";
  begin  -- procedure cache_access_test

    for i in 0 to memsize-1 loop
      cache_read(i,rdata,cache_ifc);
      mem(i) := rdata;
    end loop;  -- i

    while r > 0 or w > 0 or inv > 0 loop

      random(seed,r+w+inv,rand);
      --write(output,"w: " & integer'image(w) & " r: " & integer'image(r) & " inv: " & integer'image(inv) & LF & "  rand: " & integer'image(rand) & LF);

      if inv > 0 and rand >= (r+w) then
        inv_now := true;
        random(seed,memsize,addr);
        cache_ifc.inv_addr <= to_unsigned(addr,32);
        cache_ifc.inv_en <= '1';
        inv := inv-1;
        cache_ifc.inv_en <= '0' after 20 ns;

        random(seed,r+w+inv+1,rand);
        if rand > r+w then
          wait until rising_edge(cache_ifc.clk);
          cache_ifc.inv_en <= '0';
          next;
        end if;
      else
        inv_now := false;
      end if;

      if r > 0 and rand < r then
        random(seed,memsize,addr);
        cache_read(addr,rdata,cache_ifc);
        r := r-1;
        assert rdata = mem(addr) report "read from cache at " & integer'image(addr) & "  " & integer'image(to_integer(unsigned(mem(addr)))) & " failed" severity error;
      elsif w > 0 and rand >= r and rand < (r+w) then
        random(seed,memsize,addr);
        cache_write(addr,wdata,cache_ifc);
        mem(addr) := wdata;
        wdata := to_stdUlogicVector(std_logic_vector(unsigned(wdata)+1));
        w := w-1;
      end if;

      --cache_ifc.inv_en <= '0';
    end loop;

    for i in 0 to memsize-1 loop
      cache_read(i,rdata,cache_ifc);
      assert rdata = mem(i) report "final data check at " & integer'image(i) & " failed" severity error;
    end loop;  -- i
    
  end procedure cache_access_test;

  -- component generics
  constant WordsPerLine  : natural := 8;
  constant NumberOfLines : natural := 48;
  constant WriteTrough   : boolean := false;

  -- component ports
  signal clk            : std_ulogic := '0';
  signal reset_n        : std_ulogic;
  signal dout0          : std_ulogic_vector(31 downto 0);
  signal complete0      : std_ulogic;
  signal err            : std_ulogic;
  signal master0_out_in  : wb_master_ifc_in_t;
  signal master0_out_out : wb_master_ifc_out_t;
  signal dout1          : std_ulogic_vector(31 downto 0);
  signal complete1      : std_ulogic;
  signal master1_out_in  : wb_master_ifc_in_t;
  signal master1_out_out : wb_master_ifc_out_t; 

  signal cache0 : cache_ifc_t;
  signal cache1 : cache_ifc_t;

  signal mem0 : mem_t(511 downto 0) := (others => (others => '1'));
  signal mem1 : mem_t(511 downto 0) := (others => (others => '1'));

  signal mem1_we : std_ulogic;
  signal mem1_addr : unsigned(8 downto 0);
  signal mem1_data : std_ulogic_vector(31 downto 0);

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
      dout_o          => dout0,
      en_i            => cache0.en,
      we_i            => cache0.we,
      complete_o      => complete0,
      err_o           => open,
      snooping_addr_i => to_unsigned(0,32),
      snooping_en_i   => '0',
      master_out_i    => master0_out_in,
      master_out_o    => master0_out_out);

   -- component instantiation
  DUT2: entity work.wb_cache
    generic map (
      WordsPerLine  => 4,
      NumberOfLines => 16,
      WriteTrough   => true)
    port map (
      clk_i           => clk,
      reset_n_i       => reset_n,
      addr_i          => cache1.addr,
      din_i           => cache1.din,
      dout_o          => dout1,
      en_i            => cache1.en,
      we_i            => cache1.we,
      complete_o      => complete1,
      err_o           => open,
      snooping_addr_i => cache1.inv_addr,
      snooping_en_i   => cache1.inv_en,
      master_out_i    => master1_out_in,
      master_out_o    => master1_out_out);

  process is
  begin  -- process
    master0_out_in.dat <= (others => '0');
    master0_out_in.ack <= '0';
    master0_out_in.stall <= '0';
    master0_out_in.err <= '0';
    wait until rising_edge(clk) and master0_out_out.cyc = '1' and master0_out_out.stb = '1';
    master0_out_in.ack <= '1';
    if master0_out_out.we = '1' then
      mem0(to_integer(master0_out_out.adr)) <= master0_out_out.dat;
    else
      master0_out_in.dat <= mem0(to_integer(master0_out_out.adr));
    end if;
    wait until rising_edge(clk);      
  end process;

  process is
  begin  -- process
    master1_out_in.dat <= (others => '0');
    master1_out_in.ack <= '0';
    master1_out_in.stall <= '0';
    master1_out_in.err <= '0';
    wait until rising_edge(clk);

    if mem1_we = '1' then
      mem1(to_integer(mem1_addr)) <= mem1_data;
    end if;

    if master1_out_out.cyc = '1' and master1_out_out.stb = '1' then
      master1_out_in.ack <= '1';
      if master1_out_out.we = '1' then
        mem1(to_integer(master1_out_out.adr)) <= master1_out_out.dat;
      else
        master1_out_in.dat <= mem1(to_integer(master1_out_out.adr));
      end if;
      wait until rising_edge(clk);
    end if;
  end process;

  -- clock generation
  clk <= not clk after 10 ns;

  cache0.clk <= clk;
  cache0.dout <= std_logic_vector(dout0);
  cache0.complete <= complete0;
  cache1.clk <= clk;
  cache1.dout <= std_logic_vector(dout1);
  cache1.complete <= complete1;

  process
    variable rdata : std_ulogic_vector(31 downto 0);
    variable wdata : std_ulogic_vector(31 downto 0);
    variable startTime : time;
  begin
    reset_n <= '0';

    mem1_we <= '0';
    mem1_addr <= to_unsigned(0,9);
    mem1_data <= (others => '0');

    cache0 <= ('Z',to_unsigned(0,32),(others => '0'),(others => 'Z'),'0','0','Z',to_unsigned(0,32),'0');
    cache1 <= ('Z',to_unsigned(0,32),(others => '0'),(others => 'Z'),'0','0','Z',to_unsigned(0,32),'0');

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
    assert rdata = X"ABCDEF00" report "test that cached write to existing line works: cache line not updated" severity error;

    wait for 1us;
    
    -- test that write to invalid cache line works
    wdata := X"ABABABAB";
    startTime := now;
    cache_write(27,wdata,cache0);
    cache_read(27,rdata,cache0);
    assert (now-startTime) <= 60 ns
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
    cache_write(26,wdata,cache0); -- make dirty
    startTime := now;
    wdata := X"DEADBEEF";
    cache_write(90,wdata,cache0);
    cache_read(90,rdata,cache0);
    assert (now-startTime) <= 60 ns
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

    wait for 1 us;

    -- test that write with wb stalls correctly while pending fetch with wb
    wdata := X"00DEAD00";
    cache_write(1,wdata,cache0); -- make dirty
    wdata := X"CAFEBABE";
    cache_write(6,wdata,cache0); -- make dirty
    wdata := X"DEADB00B";
    cache_write(64,wdata,cache0); -- write to cache line 0
    wdata := X"B00BDEAD";
    cache_write(68,wdata,cache0); -- write to cache line 1
    cache_read(64,rdata,cache0);
    assert rdata = X"DEADB00B"
      report "test that write with wb stalls correctly while pending fetch with wb: cache line 0 not updated" severity error;
    cache_read(68,rdata,cache0);
    assert rdata = X"B00BDEAD"
      report "test that write with wb stalls correctly while pending fetch with wb: cache line 1 not updated" severity error;
    cache_flush_by_reload_line(64,cache0);
    cache_flush_by_reload_line(68,cache0);
    assert mem0(64) = X"DEADB00B" and mem0(68) = X"B00BDEAD"
      report "test that write with wb stalls correctly while pending fetch with wb: data is not written back correctly" severity error;

    wait for 1us;

    -- test that pending fetch do not stall read to other line
    wdata := X"99876543";
    cache_write(9,wdata,cache0);
    cache_flush_by_reload_line(0,cache0);
    cache_read(0,rdata,cache0); -- causes fetch
    startTime := now;
    cache_read(9,rdata,cache0);
    assert (now-startTime) <= 20 ns
      report "test that pending fetch do not stall read to other line: read takes too much time" severity error;
    assert rdata = X"99876543"
      report "test that pending fetch do not stall read to other line: read failed" severity error;

    wait for 1us;

    -- test that pending fetch do not stall write to other line
    cache_read(18,rdata,cache0);
    cache_flush_by_reload_line(13,cache0);
    cache_read(13,rdata,cache0); -- casues fetch
    startTime := now;
    wdata := X"9987EEEE";
    cache_write(18,wdata,cache0);
    assert (now-startTime) <= 40ns
      report "test that pending fetch do not stall write to other line: write takes too much time" severity error;
    cache_read(18,rdata,cache0);
    assert rdata = X"9987EEEE"
      report "test that pending fetch do not stall write to other line: write failed" severity error;

    -- test that pending writeback do not stall read to other line
    wdata := X"DDEADBEE";
    cache_write(35,wdata,cache0);
    wdata := X"DEADBEE2";
    cache_write(37,wdata,cache0);
    cache_read(37+64,rdata,cache0); -- causes writeback
    startTime := now;
    cache_read(35,rdata,cache0);
    assert (now-startTime) <= 20 ns
      report "test that pending writeback do not stall read to other line: read takes too much time" severity error;
    assert rdata = X"DDEADBEE"
      report "test that pending writeback do not stall read to other line: read failed" severity error;

    wait for 1us;

    -- test that pending writeback do not stall write to other line
    cache_read(48,rdata,cache0);
    wdata := X"EADBBBEE";
    cache_write(45,wdata,cache0);
    wdata := X"DEDEDEDE";
    cache_write(45+64,wdata,cache0); -- causes writeback
    startTime := now;
    wdata := X"EADBBEE2";
    cache_write(48,wdata,cache0);
    assert (now-startTime) <= 40ns
      report "test that pending writeback do not stall write to other line: write takes too much time" severity error;
    cache_read(48,rdata,cache0);
    assert rdata = X"EADBBEE2"
      report "test that pending writeback do not stall write to other line: write failed" severity error;

    -- test that idle write only takes on cycle
    cache_read(54,rdata,cache0);
    wait for 200 ns;
    startTime := now;
    wdata := X"BEDBED99";
    cache_write(54,wdata,cache0);
    assert (now-startTime) <= 20ns
      report "test that idle write only takes on cycle: write takes too much time" severity error;
    cache_read(54,rdata,cache0);
    assert rdata = X"BEDBED99"
      report "test that idle write only takes on cycle: write failed" severity error;


    wait for 1 us;


    -- test that write through works with invalid line
    startTime := now;
    wdata := X"ABC00000";
    cache_write(3,wdata,cache1);
    assert (now-startTime) <= 20ns
      report "test that write through works with invalid line: write takes too much time" severity error;
    wait for 20 ns;
    assert mem1(3) = X"ABC00000"
      report "test that write through works with invalid line: write through failed" severity error;
    cache_read(3,rdata,cache1);
    assert rdata = X"ABC00000"
      report "test that write through works with invalid line: cache line not updated" severity error;

    wait for 1 us;

    -- test that write works while fetching data
    wdata := X"ABCD9999";
    cache_write(7,wdata,cache1);
    cache_flush_by_reload_line(7,cache1);
    cache_read(7,rdata,cache1);
    wdata := X"ABCD7777";
    cache_write(10,wdata,cache1);
    wait for 20 ns;
    assert mem1(10) = X"ABCD7777"
      report "test that write works while fetching data: write through failed" severity error;
    cache_read(10,rdata,cache1);
    assert rdata = X"ABCD7777"
      report "test that write works while fetching data: cache line not updated" severity error;

    wait for 1 us;

    -- test that write through write through to same line works
    wdata := X"A123456B";
    cache_write(12,wdata,cache1);
    wdata := X"A123456C";
    cache_write(13,wdata,cache1);
    wait for 20 ns;
    assert mem1(12) = X"A123456B" and mem1(13) = X"A123456C"
      report "test that write through write through to same line works: write through failed" severity error;
    cache_read(12,rdata,cache1);
    assert rdata = X"A123456B"
      report "test that write through write through to same line works: cache line not updated" severity error;
    cache_read(13,rdata,cache1);
    assert rdata = X"A123456C"
      report "test that write through write through to same line works: cache line not updated" severity error;

    wait for 1 us;

    -- test that write through write through to different line works
    wdata := X"A123456D";
    cache_write(14,wdata,cache1);
    wdata := X"A123456E";
    cache_write(16,wdata,cache1);
    wait for 20 ns;
    assert mem1(14) = X"A123456D" and mem1(16) = X"A123456E"
      report "test that write through write through to different line works: write through failed" severity error;
    cache_read(14,rdata,cache1);
    assert rdata = X"A123456D"
      report "test that write through write through to different line works: cache line0 not updated" severity error;
    cache_read(16,rdata,cache1);
    assert rdata = X"A123456E"
      report "test that write through write through to different line works: cache line not updated" severity error;

    wait for 1 us;

    -- test that write through followed by fetched read works
    cache_read(21,rdata,cache1);
    wdata := X"A123456F";
    cache_write(22,wdata,cache1);
    startTime := now;
    cache_read(22,rdata,cache1);
    assert (now-startTime) <= 20ns
      report "test that write through followed by fetched read works: read takes too much time" severity error;
    assert rdata = X"A123456F"
      report "test that write through followed by fetched read works: cache line not updated" severity error;    
    assert mem1(22) = X"A123456F"
      report "test that write through followed by fetched read works: write through failed" severity error;

    wait for 1 us;

    -- test that write through followed by read works
    wdata := X"A123B123";
    cache_write(27,wdata,cache1);
    wait for 60 ns;
    startTime := now;
    cache_read(27,rdata,cache1);
    assert (now-startTime) <= 120ns
      report "test that write through followed by read works: read takes too much time" severity error;
    assert rdata = X"A123B123"
      report "test that write through followed by read works: cache line not updated" severity error;    
    assert mem1(27) = X"A123B123"
      report "test that write through followed by read works: write through failed" severity error;

    wait for 1 us;


    -- test that invalidate while idle works
    cache_read(33,rdata,cache1);
    wait until rising_edge(clk);
    mem1_we <= '1';
    mem1_addr <= to_unsigned(33,9);
    mem1_data <= X"DCBA9913";
    wait until rising_edge(clk);
    mem1_we <= '0';
    wait for 100 ns;
    invalidate_addr(33,cache1);
    wait for 40 ns;
    cache_read(33,rdata,cache1);
    assert rdata = X"DCBA9913"
      report "test that invalidate while idle works: cache line reload did not work" severity error;

    -- test that invalidate while fetching works
    cache_read(36,rdata,cache1);
    wait until rising_edge(clk);
    mem1_we <= '1';
    mem1_addr <= to_unsigned(36,9);
    mem1_data <= X"DC339913";
    wait until rising_edge(clk);
    mem1_we <= '0';
    invalidate_addr(36,cache1);
    wait for 40 ns;
    cache_read(36,rdata,cache1);
    assert rdata = X"DC339913"
      report "test that invalidate while fetching works: cache line reload did not work" severity error;

    -- test that invalidate while reading works
    cache_read(40,rdata,cache1);
    wait until rising_edge(clk);
    mem1_we <= '1';
    mem1_addr <= to_unsigned(40,9);
    mem1_data <= X"DCBBBB00";
    wait until rising_edge(clk);
    mem1_we <= '0';
    cache1.inv_addr <= to_unsigned(40,32);
    cache1.inv_en <= '1';
    --cache1.inv_en <= '0' after 40 ns;
    cache_read(41,rdata,cache1);
    cache1.inv_en <= '0';
    wait for 40 ns;
    cache_read(40,rdata,cache1);
    assert rdata = X"DCBBBB00"
      report "test that invalidate while reading works: cache line reload did not work" severity error;


    startTime := now;
    cache_access_test(1024*16,512*16,433*16,256,cache0);
    write(output,"cache0 total access cycles per operation " & integer'image(((now-startTime)/20 ns)/(1536*16)) & LF);

    
    startTime := now;
    cache_access_test(1024*16,512*16,433*16,256,cache1);
    write(output,"cache1 total access cycles per operation  " & integer'image(((now-startTime)/20 ns)/(1536*16)) & LF);


    wait for 100 ns;
    write(output,"all tests complete" & LF);
    reset_n <= '0';
    
    wait;
    
  end process;

  

end architecture behav;
