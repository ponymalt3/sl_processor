library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package wb_test_p is

  type master_out_t is record
    addr     : unsigned(31 downto 0);
    dout     : std_ulogic_vector(31 downto 0);
    en       : std_ulogic;
    we       : std_ulogic;
    burst    : unsigned(5 downto 0);
  end record master_out_t;

  type master_in_t is record
    din      : std_ulogic_vector(31 downto 0);
    dready   : std_ulogic;
    complete : std_ulogic;
    err      : std_ulogic;
  end record master_in_t;

  type data_array_t is array (natural range <>) of std_ulogic_vector(31 downto 0);

  procedure master_access_burst (
    addr  : in  natural;
    din   : in  data_array_t;
    dout  : out data_array_t;
    we    : in  std_ulogic;
    burst : in unsigned(2 downto 0);
    signal master_in : in master_in_t;
    signal master_out : out master_out_t;
    clk_period : in time := 20 ns);

  procedure master_access (
    addr : in  natural;
    din  : in  std_ulogic_vector(31 downto 0);
    dout : out std_ulogic_vector(31 downto 0);
    we   : in  std_ulogic;
    signal master_in : in master_in_t;
    signal master_out : out master_out_t;
    clk_period : in time := 20 ns);

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
    signal cache_ifc : inout cache_ifc_t);

  procedure cache_read (
    addr             : in    natural;
    dout             : out   std_ulogic_vector(31 downto 0);
    signal cache_ifc : inout cache_ifc_t);

  procedure invalidate_addr (
    addr             : in natural;
    signal cache_ifc : inout cache_ifc_t);

  procedure cache_flush_by_reload_line (
    addr             : in natural;
    signal cache_ifc : inout cache_ifc_t);

end package wb_test_p;

package body wb_test_p is

  procedure master_access_burst (
    addr  : in  natural;
    din   : in  data_array_t;
    dout  : out data_array_t;
    we    : in  std_ulogic;
    burst : in unsigned(2 downto 0);
    signal master_in : in master_in_t;
    signal master_out : out master_out_t;
    clk_period : in time := 20 ns) is

    variable index : natural := 0;
  begin
    master_out.addr <= to_unsigned(addr,32);
    master_out.dout <= din(0);
    master_out.en <= '1';
    master_out.we <= we;
    master_out.burst <= "000" & burst;

    wait for clk_period-1 ps;

    while master_in.complete = '0' loop      
      
      if master_in.dready = '1' and index < din'length then
        dout(index) := master_in.din;
      end if;
      
      index := index+1;
      
      if master_in.dready = '1' and index < din'length then
        master_out.dout <= din(index);
      end if;

      wait for clk_period;
    end loop;

    master_out.en <= '0';

    wait for 1 ps;
  end procedure;

  procedure master_access (
    addr : in  natural;
    din  : in  std_ulogic_vector(31 downto 0);
    dout : out std_ulogic_vector(31 downto 0);
    we   : in  std_ulogic;
    signal master_in : in master_in_t;
    signal master_out : out master_out_t;
    clk_period : in time := 20 ns) is

    variable din_array : data_array_t(0 downto 0);
    variable dout_array : data_array_t(0 downto 0);
  begin    
    din_array(0) := din;
    master_access_burst(addr,din_array,dout_array,we,to_unsigned(0,3),master_in,master_out,clk_period);
    dout := dout_array(0);
  end procedure;

  
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

end package body wb_test_p;
