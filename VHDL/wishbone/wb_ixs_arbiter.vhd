library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;

entity wb_interconnect_arbiter is
  
  generic (
    MasterConfig : wb_master_config_array_t);

  port (
    clk_i     : in  std_ulogic;
    reset_n_i : in  std_ulogic;
    master_in_i : in wb_slave_ifc_in_array_t(MasterConfig'length-1 downto 0);
    master_in_o : out wb_slave_ifc_out_array_t(MasterConfig'length-1 downto 0);
    master_out_i  : in wb_master_ifc_in_t;
    master_out_o  : out wb_master_ifc_out_t);

end entity wb_interconnect_arbiter;

architecture rtl of wb_interconnect_arbiter is

  constant NumMaster : natural := MasterConfig'length;

  signal master_sel : integer range 0 to 32;
  signal master_sel_reg : integer range 0 to 32;
  signal mask : std_ulogic_vector(NumMaster-1 downto 0);
  
begin  -- architecture rtl

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      mask <= (others => '1');
      master_sel_reg <= NumMaster;
    elsif falling_edge(clk_i) then  -- falling clock edge

      master_sel_reg <= master_sel;
      
      if master_sel /= master_sel_reg then
        if master_sel_reg < NumMaster then
          mask(master_sel_reg) <= '0';
        end if;
      end if;

      if master_sel = NumMaster then
        mask <= (others => '1');
      end if;
      
    end if;
  end process;

  process (mask, master_in_i, master_out_i, master_sel_reg) is
  begin  -- process

    master_sel <= NumMaster;
    for i in 0 to NumMaster-1 loop
      
      --default output values
      master_in_o(i).dat <= (others => '0');
      master_in_o(i).ack <= '0';
      master_in_o(i).err <= '0';
      master_in_o(i).stall <= '1';
      
      if master_in_i(i).cyc = '1' and mask(i) = '1' then
        master_sel <= i;
      end if;
    end loop;  -- i

    master_out_o.cyc <= '0';
    master_out_o.adr <= to_unsigned(0,32);
    master_out_o.dat <= (others => '0');
    master_out_o.we  <= '0';
    master_out_o.sel <= (others => '0');
    master_out_o.stb <= '0';

    if master_sel_reg < NumMaster then
      master_out_o <= master_in_i(master_sel_reg);
      master_in_o(master_sel_reg) <= master_out_i;
    end if;
    
  end process;

end architecture rtl;
