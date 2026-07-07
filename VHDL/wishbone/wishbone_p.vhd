library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

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
    variable hash : unsigned(31 downto 0) := to_unsigned(46929344, 32);
    variable tmp  : unsigned(63 downto 0);
  begin
    for i in name'range loop
      tmp  := hash * to_unsigned(89428428, 32);
      hash := tmp(31 downto 0) + to_unsigned(character'pos(name(i)), 32);
    end loop;
    return to_integer(signed(hash));
  end function;

  function wb_master (
    connected_slaves : string)
    return wb_master_config_t is
    variable cur_slave : natural;
    variable pos : natural;
    variable slave_hashes : integer_array_t(31 downto 0) := (others => 0);
  begin
    cur_slave := 0;
    pos := 1;
    for i in 1 to connected_slaves'length loop

      if connected_slaves(i) = ' '  then
        slave_hashes(cur_slave) := wb_hash(connected_slaves(pos to i-1));
        pos := i+1;
        cur_slave := cur_slave+1;
      elsif i = connected_slaves'length then
        slave_hashes(cur_slave) := wb_hash(connected_slaves(pos to i));
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
  begin
    assert size /= 0 report "slave addr range size cant be zero" severity error;
    return (wb_hash(name),to_unsigned(addr,32),to_unsigned(size,32),255);
  end function;

  function wb_config_find_slave (
    slave_hash : integer;
    slave_list : wb_slave_config_array_t)
    return wb_slave_config_t is
    variable result : wb_slave_config_t;
  begin
    
    result.id := 255;
    for i in 0 to slave_list'length-1 loop
      if slave_list(i).hash = slave_hash then
        result := slave_list(i);
        result.id := i;
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
  begin
    num_master := 0;
    for i in 0 to master_list'length-1 loop
      slave_id := wb_config_find_slave(slave_hash,wb_config_get_connected_slaves(master_list(i),slave_list)).id;
      if slave_id /= 255 then
        result(num_master) := master_list(i);
        result(num_master).id := i;
        result(num_master).rel_slave_pos := slave_id;
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
  begin
    i := 0;
    while i < slaves'length-1 and master.connected_slaves(i) /= 0 loop
      slaves(i) := wb_config_find_slave(master.connected_slaves(i),complete_slave_list);
      assert slaves(i).id /= 255 report "  slave not found in list" severity error;
      i := i+1;
    end loop;

    assert i > 0 report "master with no slaves not allowed" severity failure;

    return slaves(i-1 downto 0);
  end function;
    
end package body wishbone_p;
