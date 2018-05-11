library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.textio.all;

package wishbone_p is

  type wb_master_ifc_out_t is record 
    adr : unsigned(31 downto 0);
    dat : std_ulogic_vector(31 downto 0);
    we  : std_ulogic;
    sel : std_ulogic_vector(3 downto 0);
    stb : std_ulogic;
    cyc : std_ulogic;
  end record wb_master_ifc_out_t;

  type wb_master_ifc_out_array_t is array (natural range <>) of wb_master_ifc_out_t;

  type wb_master_ifc_in_t is record
    dat : std_ulogic_vector(31 downto 0);
    ack : std_ulogic;
  end record wb_master_ifc_in_t;

  type wb_master_ifc_in_array_t is array (natural range <>) of wb_master_ifc_in_t;

  subtype wb_slave_ifc_in_t is wb_master_ifc_out_t;
  subtype wb_slave_ifc_out_t is wb_master_ifc_in_t;

  type wb_slave_ifc_out_array_t is array (natural range <>) of wb_slave_ifc_out_t;
  type wb_slave_ifc_in_array_t is array (natural range <>) of wb_slave_ifc_in_t;
 
  type wb_master_config_t is record
    id        : natural;
    connected_slaves : string(1 to 128);
    rel_slave_pos : natural; -- only used if masters are filter for one slave
  end record wb_master_config_t;

  type wb_master_config_array_t is array (natural range <>) of wb_master_config_t;

  type wb_slave_config_t is record
    name : string(1 to 32);
    addr : unsigned(31 downto 0);
    size : unsigned(31 downto 0);
    id : natural;
  end record wb_slave_config_t;

  type wb_slave_config_array_t is array (natural range <>) of wb_slave_config_t;

  function wb_interconnect_get_connected_slaves (
    master              : wb_master_config_t;
    complete_slave_list : wb_slave_config_array_t)
    return wb_slave_config_array_t;

  function wb_interconnect_find_slave (
    slave_name : string;
    slave_list : wb_slave_config_array_t)
    return wb_slave_config_t;

  function wb_interconnect_get_masters_for_slave (
    slave_name : string;
    slave_list : wb_slave_config_array_t;
    master_list : wb_master_config_array_t)
    return wb_master_config_array_t;

  function wb_master (
    connected_slaves : string)
    return wb_master_config_t;

  function wb_slave (
    name : string;
    addr : natural;
    size : natural)
    return wb_slave_config_t;
  
end package wishbone_p;

package body wishbone_p is

  function wb_master (
    connected_slaves : string)
    return wb_master_config_t is
    variable result : string(1 to 128) := (others => NUL);
  begin
    result(1 to connected_slaves'length) := connected_slaves;
    return (255,result,255);
  end function;

  function wb_slave (
    name : string;
    addr : natural;
    size : natural)
    return wb_slave_config_t is
    variable result : string(1 to 32) := (others => NUL);
  begin
    result(1 to name'length) := name;
    return (result,to_unsigned(addr,32),to_unsigned(size,32),255);
  end function;

  function wb_interconnect_find_slave (
    slave_name : string;
    slave_list : wb_slave_config_array_t)
    return wb_slave_config_t is
    variable result : wb_slave_config_t;
  begin
    result.id := 255;
    for i in 0 to slave_list'length-1 loop
      if slave_list(i).name(1 to slave_name'length) = slave_name then
        result := slave_list(i);
        result.id := i;
        return result;
      end if;
    end loop;  -- i
    return result;    
  end function;

  function wb_interconnect_get_masters_for_slave (
    slave_name : string;
    slave_list : wb_slave_config_array_t;
    master_list : wb_master_config_array_t)
    return wb_master_config_array_t is
    variable slave_id : natural;
    variable result : wb_master_config_array_t(31 downto 0);
    variable num_master : natural;
    file output : text open write_mode is "STD_OUTPUT";
  begin
    write(output,"enumerating master for slave " & slave_name & LF); 
    num_master := 0;
    for i in 0 to master_list'length-1 loop
      slave_id := wb_interconnect_find_slave(slave_name,wb_interconnect_get_connected_slaves(master_list(i),slave_list)).id;
      if slave_id /= 255 then
        result(num_master) := master_list(i);
        result(num_master).id := i;
        result(num_master).rel_slave_pos := slave_id;
        --write(output,"  id: " & integer'image(result(num_master).id) & "  slave pos: " & integer'image(result(num_master).rel_slave_pos) & LF); 
        num_master := num_master+1;        
      end if;
    end loop;  -- i

    assert num_master > 0 report "slave without master not allowed" severity failure;

    return result(num_master-1 downto 0);
  end function;
  
  function wb_interconnect_get_connected_slaves (
    master              : wb_master_config_t;
    complete_slave_list : wb_slave_config_array_t)
    return wb_slave_config_array_t is
    
    variable slaves : wb_slave_config_array_t(31 downto 0);
    variable slave_name : string(1 to 32);
    variable pos : natural;
    variable i : natural;
    variable cur_slave : natural;
    file output : text open write_mode is "STD_OUTPUT";
    
  begin

    write(output,"enumerating master " & integer'image(master.id) & LF);    
    cur_slave := 0;     
    pos := 1;
    while master.connected_slaves(pos) /= NUL loop
      i := 1;
      while master.connected_slaves(pos) /= NUL and master.connected_slaves(pos) /= ' ' loop
        write(output,"pos. " & integer'image(pos) & "  i: " & integer'image(i) & "  str length: " & integer'image(master.connected_slaves'length) & LF);
        slave_name(i) := master.connected_slaves(pos);
        i := i+1;
        pos := pos+1;
      end loop;
      pos := pos+1;

      write(output,"  found slave " & slave_name(1 to i) & LF);     

      slaves(cur_slave) := wb_interconnect_find_slave(slave_name(1 to i),complete_slave_list);
      assert slaves(cur_slave).id /= 255 report "  slave " & slave_name & " not found in slave list" severity error;
      cur_slave := cur_slave+1;
    end loop;

    assert cur_slave > 0 report "master with no slaves not allowed" severity failure;

    return slaves(cur_slave-1 downto 0);
  end function;
    
end package body wishbone_p;

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;

use work.sl_misc_p.all;

entity wb_interconnect_decoder is
  
  generic (
    SlaveMap : wb_slave_config_array_t);

  port (
    clk_i     : in  std_ulogic;
    reset_n_i : in  std_ulogic;
    master_in_i  : in wb_slave_ifc_in_t;
    master_in_o  : out wb_slave_ifc_out_t;
    master_out_i : in wb_master_ifc_in_array_t(SlaveMap'length-1 downto 0);
    master_out_o : out wb_master_ifc_out_array_t(SlaveMap'length-1 downto 0));

end entity wb_interconnect_decoder;

architecture rtl of wb_interconnect_decoder is

  signal slave_sel : integer;
  
begin  -- architecture rtl

  process (master_in_i, master_out_i) is
  begin  -- process
    slave_sel <= 0;
    master_in_o <= ((others => '0'),'0');
    
    for i in 0 to SlaveMap'length-1 loop
      master_out_o(i) <= master_in_i;
      master_out_o(i).cyc <= '0';
      master_out_o(i).adr <= (others => '0');
      master_out_o(i).adr(log2(to_integer(SlaveMap(i).size))-1 downto 0) <= master_in_i.adr(log2(to_integer(SlaveMap(i).size))-1 downto 0);
      if master_in_i.adr >= SlaveMap(i).addr and master_in_i.adr < (SlaveMap(i).addr + SlaveMap(i).size) then
        slave_sel <= i;
        master_out_o(i).cyc <= master_in_i.cyc;
        master_in_o <= master_out_i(i);
        exit;
      end if;
    end loop;  -- i
  end process;

end architecture rtl;
