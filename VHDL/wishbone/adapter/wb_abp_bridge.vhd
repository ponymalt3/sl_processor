library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

package apb_p is

  type apb_data_t is array (natural range <>) of std_ulogic_vector(7 downto 0);
  type apb_peripheral_t is record
    low : std_ulogic_vector(15 downto 0);
    high : std_ulogic_vector(15 downto 0);
  end record;
                           
  type apb_peripheral_list_t is array (natural range <>) of apb_peripheral_t;

 -- type apb_action_t is (APB_NONE,APB_WRITE,APB_READ);

  type apb_request_t is record
    addr : std_ulogic_vector(15 downto 0);
    wr : std_ulogic;
    en : std_ulogic;
  end record;

  function SlaveEntry (
    constant base_addr : integer;
    constant size : integer)
    return apb_peripheral_t;
  
end apb_p;

package body apb_p is

   function SlaveEntry (
    constant base_addr : integer;
    constant size : integer)
    return apb_peripheral_t is
    begin
      return (To_StdUlogicVector(std_logic_vector(to_unsigned(base_addr,8))),
              To_StdUlogicVector(std_logic_vector(to_unsigned(base_addr,8)+to_unsigned(size,7))));
  end;                          

end apb_p;


library ieee;
library work;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.apb_p.all;

entity wb_abp_bridge is
  
  generic (
    Peripherals : apb_peripheral_list_t);

  port (
    clk_i      : in  std_ulogic;
    reset_n_i  : in  std_ulogic;

    wb_slave_i : in wb_slave_ifc_in_t;
    wb_slave_o : in wb_slave_ifc_out_t;
    
    apb_sel_o  : out std_ulogic_vector(Peripherals'range);
    apb_en_o   : out std_ulogic;
    apb_addr_o : out std_ulogic_vector(15 downto 0);
    apb_we_o   : out std_ulogic;
    apb_din_i  : in apb_data_t(Peripherals'range);
    apb_dout_o : out std_ulogic_vector(31 downto 0));

end wb_abp_bridge;

architecture rtl of wb_abp_bridge is

  type fsm_t is (FSM_IDLE, FSM_SETUP, FSM_ENABLE);

  signal state : fsm_t;
  signal addr : integer range 0 to 65535;
  signal din_mux : integer range 0 to Peripherals'high;
  signal active : integer  range 0 to Peripherals'high+1;
  signal dout_reg : std_ulogic_vector(7 downto 0);
  signal active_din : std_ulogic_vector(7 downto 0);

  constant SLV_NONE : integer := Peripherals'high+1;

  type apb_sig is record
    we   : std_ulogic;
    dout : std_ulogic_vector(31 downto 0);
    en   : std_ulogic;
    sel  : std_ulogic_vector(Peripherals'range);
    addr : std_ulogic_vector(15 downto 0);
  end record;
                  
  signal sig : apb_sig;
  
begin  -- rtl

  process (clk_i, reset_n_i)
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      state <= FSM_IDLE;
      apb_en_o <= '0';
      apb_we_o <= '0';
      apb_sel_o <= (others => '0');
      apb_dout_o <= (others => '0');
      apb_addr_o <= (others => '0');
      dout_reg <= (others => '0');
      din_mux <= 0;
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      if state = FSM_ENABLE then
        apb_en_o <= '0';
        dout_reg <= active_din;--dout_o <= apb_din_i(din_mux);
        apb_sel_o <= (others => '0');
        state <= FSM_IDLE;
      end if;
      if state = FSM_SETUP then
        apb_en_o <= '1';
        state <= FSM_ENABLE;
        --din_mux <= active;
      elsif en_i = '1' and active /= SLV_NONE then
        state <= FSM_SETUP;
        din_mux <= active;
        apb_we_o <= sig.we;
        apb_dout_o <= sig.dout;
        apb_addr_o <= sig.addr;
        apb_sel_o <= sig.sel;
      end if;   
    end if;
  end process;

   process (active, active_din, addr_i, apb_din_i, din_i,
            din_mux, dout_reg, state, we_i)
  begin  -- process

    sig <= ('0', (others => '0'), '0', (others => '0'), (others => '0'));

    if active /= SLV_NONE then
      sig.we <= we_i;
      sig.dout <= din_i;
      sig.addr <= To_StdUlogicVector(std_logic_vector(unsigned(addr_i)-unsigned(Peripherals(active).low)));
      sig.sel(active) <= '1';
    end if;
    
    dout_o <= dout_reg;
    if state = FSM_ENABLE then
      dout_o <= active_din;
    end if;

    active_din <= apb_din_i(din_mux);
    
    active <= SLV_NONE;
    for i in 0 to Peripherals'high loop
      if addr_i >= Peripherals(i).low and addr_i <= Peripherals(i).high then
        active <= i;
      end if;
    end loop;  -- i
    
  end process;
  
  idle_o <= '1' when state = FSM_IDLE or state = FSM_ENABLE else '0';

end rtl;


