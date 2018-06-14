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
    err : std_ulogic;
    stall : std_ulogic;
  end record wb_master_ifc_in_t;

  type wb_master_ifc_in_array_t is array (natural range <>) of wb_master_ifc_in_t;

  subtype wb_slave_ifc_in_t is wb_master_ifc_out_t;
  subtype wb_slave_ifc_out_t is wb_master_ifc_in_t;

  subtype wb_slave_ifc_in_array_t is wb_master_ifc_out_array_t;
  subtype wb_slave_ifc_out_array_t is wb_master_ifc_in_array_t;

  type integer_array_t is array (natural range <>) of integer;
 
  type wb_master_config_t is record
    id        : natural;
    connected_slaves : integer_array_t(31 downto 0);
    rel_slave_pos : natural; -- only used if masters are filter for one slave
  end record wb_master_config_t;

  type wb_master_config_array_t is array (natural range <>) of wb_master_config_t;

  type wb_slave_config_t is record
    hash : integer;
    addr : unsigned(31 downto 0);
    size : unsigned(31 downto 0);
    id : natural;
  end record wb_slave_config_t;

  type wb_slave_config_array_t is array (natural range <>) of wb_slave_config_t;

  function wb_config_get_connected_slaves (
    master              : wb_master_config_t;
    complete_slave_list : wb_slave_config_array_t)
    return wb_slave_config_array_t;

  function wb_config_find_slave (
    slave_hash : integer;
    slave_list : wb_slave_config_array_t)
    return wb_slave_config_t;

  function wb_config_get_masters_for_slave (
    slave_hash : integer;
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

  function wb_hash (
    name : string)
    return integer;
  
end package wishbone_p;

package body wishbone_p is

  function wb_hash (
    name : string)
    return integer is
    variable hash : integer := 46929344;
  begin
    for i in name'range loop
      hash := hash*89428428+character'pos(name(i));
    end loop;  -- i
    return hash;
  end function;

  function wb_master (
    connected_slaves : string)
    return wb_master_config_t is
    variable cur_slave : natural;
    variable pos : natural;
    variable slave_hashes : integer_array_t(31 downto 0) := (others => 0);
    file output : text open write_mode is "STD_OUTPUT";
  begin    
    cur_slave := 0;     
    pos := 1;
    write(output,"master:" & LF);
    for i in 1 to connected_slaves'length loop

      if connected_slaves(i) = ' '  then
        slave_hashes(cur_slave) := wb_hash(connected_slaves(pos to i-1));
        write(output,"  " & connected_slaves(pos to i-1) & " [" & integer'image(slave_hashes(cur_slave)) & "]" & LF);
        pos := i+1;
        cur_slave := cur_slave+1;
      elsif i = connected_slaves'length then
        slave_hashes(cur_slave) := wb_hash(connected_slaves(pos to i));
        write(output,"  " & connected_slaves(pos to i) & " [" & integer'image(slave_hashes(cur_slave)) & "]" & LF);
        cur_slave := cur_slave+1;
      end if;

    end loop;  -- i
 
    return (255,slave_hashes,255);
  end function;

  function wb_slave (
    name : string;
    addr : natural;
    size : natural)
    return wb_slave_config_t is
    file output : text open write_mode is "STD_OUTPUT";
  begin
    write(output,"slave:" & LF & "  " & name & " [" & integer'image(wb_hash(name)) & "]" & LF);
    return (wb_hash(name),to_unsigned(addr,32),to_unsigned(addr,32),255);
  end function;

  function wb_config_find_slave (
    slave_hash : integer;
    slave_list : wb_slave_config_array_t)
    return wb_slave_config_t is
    variable result : wb_slave_config_t;
    file output : text open write_mode is "STD_OUTPUT";
  begin
    --write(output,"find slave hash:" & integer'image(slave_hash) & LF);
    
    result.id := 255;
    for i in 0 to slave_list'length-1 loop
      --write(output,"  compare " & integer'image(slave_list(i).hash) & LF);
      if slave_list(i).hash = slave_hash then
        result := slave_list(i);
        result.id := i;
        --write(output,"     => found" & LF);
        return result;
      end if;
    end loop;  -- i
    return result;    
  end function;

  function wb_config_get_masters_for_slave (
    slave_hash : integer;
    slave_list : wb_slave_config_array_t;
    master_list : wb_master_config_array_t)
    return wb_master_config_array_t is
    variable slave_id : natural;
    variable result : wb_master_config_array_t(31 downto 0);
    variable num_master : natural;
    file output : text open write_mode is "STD_OUTPUT";
  begin
    write(output,"enumerating master for slave " & integer'image(slave_hash) & LF); 
    num_master := 0;
    for i in 0 to master_list'length-1 loop
      slave_id := wb_config_find_slave(slave_hash,wb_config_get_connected_slaves(master_list(i),slave_list)).id;
      if slave_id /= 255 then
        result(num_master) := master_list(i);
        result(num_master).id := i;
        result(num_master).rel_slave_pos := slave_id;
        write(output,"  id: " & integer'image(result(num_master).id) & "  slave pos: " & integer'image(result(num_master).rel_slave_pos) & LF); 
        num_master := num_master+1;        
      end if;
    end loop;  -- i

    assert num_master > 0 report "slave without master not allowed" severity failure;

    return result(num_master-1 downto 0);
  end function;
  
  function wb_config_get_connected_slaves (
    master              : wb_master_config_t;
    complete_slave_list : wb_slave_config_array_t)
    return wb_slave_config_array_t is
    
    variable slaves : wb_slave_config_array_t(31 downto 0);
    variable i : natural;
    --file output : text open write_mode is "STD_OUTPUT";
    
  begin
    i := 0;
    while i < slaves'length-1 and master.connected_slaves(i) /= 0 loop
      slaves(i) := wb_config_find_slave(master.connected_slaves(i),complete_slave_list);
      assert slaves(i).id /= 255 report "  slave not found in slave list" severity error;
      i := i+1;
    end loop;

    assert i > 0 report "master with no slaves not allowed" severity failure;

    return slaves(i-1 downto 0);
  end function;
    
end package body wishbone_p;
