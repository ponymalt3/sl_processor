library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;
use work.sl_misc_p.all;

entity wb_cache_adapter is
  port (
    clk_i      : in    std_ulogic;
    reset_n_i  : in    std_ulogic;
    
    addr_o     : out    unsigned(31 downto 0);
    din_i      : in    std_ulogic_vector(31 downto 0);
    dout_o     : out   std_ulogic_vector(31 downto 0);
    en_o       : out   std_ulogic;
    we_o       : out    std_ulogic;
    complete_i : in   std_ulogic;
    err_i      : in   std_ulogic;
    
    slave_i : in wb_slave_ifc_in_t;
    slave_o : out wb_slave_ifc_out_t);

end entity wb_cache_adapter;

architecture rtl of wb_cache_adapter is

begin
  
    addr_o <= slave_i.adr;
    dout_o <= slave_i.dat;
    en_o <= slave_i.stb and slave_i.cyc;
    we_o <= slave_i.we;

    slave_o <= (din_i,complete_i,err_i,not complete_i);
    
end architecture rtl;
