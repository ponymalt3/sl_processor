library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;
use work.sl_misc_p.all;

library std;
use std.textio.all;

entity wb_ixs_decoder is
  
  generic (
    SlaveMap : wb_slave_config_array_t);

  port (
    clk_i     : in  std_ulogic;
    reset_n_i : in  std_ulogic;
    master_in_i  : in wb_slave_ifc_in_t;
    master_in_o  : out wb_slave_ifc_out_t;
    master_out_i : in wb_master_ifc_in_array_t(SlaveMap'length-1 downto 0);
    master_out_o : out wb_master_ifc_out_array_t(SlaveMap'length-1 downto 0));

end entity wb_ixs_decoder;

architecture rtl of wb_ixs_decoder is

begin  -- architecture rtl
  
  process (master_in_i, master_out_i) is
  begin  -- process
    master_in_o <= ((others => '0'),'0','1','0');
      
    for i in 0 to SlaveMap'length-1 loop
      master_out_o(i) <= master_in_i;
      master_out_o(i).cyc <= '0';
      master_out_o(i).adr <= (others => '0');
      master_out_o(i).adr(log2(to_integer(SlaveMap(i).size))-1 downto 0) <= master_in_i.adr(log2(to_integer(SlaveMap(i).size))-1 downto 0);
      if master_in_i.adr >= SlaveMap(i).addr and master_in_i.adr < (SlaveMap(i).addr + SlaveMap(i).size) then
        master_out_o(i).cyc <= master_in_i.cyc;
        master_in_o <= master_out_i(i);
      end if;
    end loop;  -- i

  end process;
  
end architecture rtl;
