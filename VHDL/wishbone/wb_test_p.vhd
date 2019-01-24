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
    signal master_out : out master_out_t);

  procedure master_access (
    addr : in  natural;
    din  : in  std_ulogic_vector(31 downto 0);
    dout : out std_ulogic_vector(31 downto 0);
    we   : in  std_ulogic;
    signal master_in : in master_in_t;
    signal master_out : out master_out_t);

end package wb_test_p;

package body wb_test_p is

  procedure master_access_burst (
    addr  : in  natural;
    din   : in  data_array_t;
    dout  : out data_array_t;
    we    : in  std_ulogic;
    burst : in unsigned(2 downto 0);
    signal master_in : in master_in_t;
    signal master_out : out master_out_t) is

    variable index : natural := 0;
    variable at_least_one_data_phase : boolean := false;
  begin
    master_out.addr <= to_unsigned(addr,32);
    master_out.dout <= din(0);
    master_out.en <= '1';
    master_out.we <= we;
    master_out.burst <= "000" & burst;

    while master_in.complete = '0' or not at_least_one_data_phase loop
      wait until master_in.dready = '1' or master_in.err = '1' or master_in.complete = '1';
      wait for 1 ps;
      if index < din'length then
        dout(index) := master_in.din;
      end if;
      index := index+1;
      if index < din'length then
        master_out.dout <= din(index);
      end if;
      at_least_one_data_phase := true;
    end loop;

    master_out.en <= '0';
  end procedure;

  procedure master_access (
    addr : in  natural;
    din  : in  std_ulogic_vector(31 downto 0);
    dout : out std_ulogic_vector(31 downto 0);
    we   : in  std_ulogic;
    signal master_in : in master_in_t;
    signal master_out : out master_out_t) is

    variable din_array : data_array_t(0 downto 0);
    variable dout_array : data_array_t(0 downto 0);
  begin    
    din_array(0) := din;
    master_access_burst(addr,din_array,dout_array,we,to_unsigned(0,3),master_in,master_out);
    dout := dout_array(0);
  end procedure;

end package body wb_test_p;
