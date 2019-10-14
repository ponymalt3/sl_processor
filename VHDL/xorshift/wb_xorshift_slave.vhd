library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

library work;
use work.wishbone_p.all;

entity wb_xorshift_slave is
  
  port (
    clk_i : in std_ulogic;
    reset_n_i : in std_ulogic;

    slave_i : in wb_slave_ifc_in_t;
    slave_o : out wb_slave_ifc_out_t);
end wb_xorshift_slave;

architecture rtl of wb_xorshift_slave is
  
  signal state : std_ulogic_vector(31 downto 0);
  
begin  -- rtl

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      state <= X"34aeb348";
      slave_o <= ((others => '0'),'0','0','1');
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      slave_o.ack <= '0';
      slave_o.stall <= '0';
      slave_o.err <= '0';
      slave_o.dat <= state;

      if slave_i.cyc = '1' and slave_i.stb = '1' then
        if slave_i.adr < to_unsigned(4,32) then
          slave_o.ack <= '1';
          state <= state xor
                   (state(18 downto 0) & (12 downto 0 => '0')) xor
                   ((16 downto 0 => '0') & state(31 downto 17)) xor
                   (state(26 downto 0) & (4 downto 0 => '0'));

          case slave_i.adr(1 downto 0) is
            when to_unsigned(0,2) => slave_o.dat <= X"00" & state(31 downto 8); -- qfp [0,1)
            when to_unsigned(1,2) => slave_o.dat <= state(7) & "0000000" & state(31 downto 8); -- qfp (-1,1)
            when to_unsigned(2,2) => slave_o.dat <= state; -- qfp (-512*2^20,512*2^20)
            when to_unsigned(3,2) => state <= slave_i.dat; -- set seed
            when others => null;
          end case;
        else
          slave_o.err <= '1';
        end if;        
      end if;
      
    end if;
  end process;

end rtl;
