library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;

entity wb_master is
  
  port (
    clk_i      : in    std_ulogic;
    reset_n_i  : in    std_ulogic;
    
    addr_i     : in    unsigned(31 downto 0);
    din_i      : in    std_ulogic_vector(31 downto 0);
    dout_o     : out   std_ulogic_vector(31 downto 0);
    en_i       : in    std_ulogic;
    we_i       : in    std_ulogic;
    complete_o : out   std_ulogic;
    err_o      : out   std_ulogic;
    
    master_out_i : in wb_master_ifc_in_t;
    master_out_o : out wb_master_ifc_out_t);

end entity wb_master;

architecture rtl of wb_master is

  type fsm_t is (ST_IDLE,ST_PENDING);
  signal state : fsm_t;

begin  -- architecture rtl

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      state <= ST_IDLE;
      master_out_o <= (to_unsigned(0,32),(others => '0'),'0',"0000",'0','0');
      dout_o <= (others => '0');
      complete_o <= '0';
      err_o <= '0';
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      complete_o <= '0';
      err_o <= master_out_i.err;
      
      if en_i = '1' and state = ST_IDLE then
        state <= ST_PENDING;
        master_out_o.adr <= addr_i;
        master_out_o.dat <= din_i;
        master_out_o.sel <= (others => '1');
        master_out_o.we <= we_i;
        master_out_o.stb <= '1';
        master_out_o.cyc <= '1';
      elsif state = ST_PENDING then
        if master_out_i.stall = '0' then
          master_out_o.stb <= '0';
        end if;
        if master_out_i.ack = '1' or master_out_i.err = '1' then
          state <= ST_IDLE;
          dout_o <= master_out_i.dat;
          master_out_o.stb <= '0';
          master_out_o.cyc <= '0';
          complete_o <= '1';
        end if;
      end if;
    end if;
  end process;
  
end architecture rtl;
