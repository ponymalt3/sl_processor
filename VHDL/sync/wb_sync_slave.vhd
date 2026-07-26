library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

library work;
use work.wishbone_p.all;
use work.sl_misc_p.all;

entity wb_sync_slave is
  generic (
    NumSyncSlots : natural := 16);

  port (
    clk_i : in std_ulogic;
    reset_n_i : in std_ulogic;

    slave_i : in wb_slave_ifc_in_t;
    slave_o : out wb_slave_ifc_out_t);
end wb_sync_slave;

architecture rtl of wb_sync_slave is

  type slot_array_t is array (natural range <>) of std_ulogic_vector(7 downto 0);

  signal slave_in : wb_slave_ifc_in_t;
  signal slots : slot_array_t(NumSyncSlots-1 downto 0);

begin  -- rtl

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      slave_o <= ((others => '0'),'0','0','1');
      slave_in <= (to_unsigned(0,32),(others => '0'),'0',(others => '0'),'0','0');
      slots <= (others => (others => '0'));
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      slave_in <= slave_i;

      slave_o.ack <= '0';
      slave_o.stall <= '0';
      slave_o.err <= '0';
      slave_o.dat <= mem_dout;

      if slave_in.cyc = '1' and slave_i.stb = '1' then
        if slave_in.addr < to_unsigned(NumSyncSlots,32) then
          slave_o.ack <= '1';
          sync_o <= slave_in.we;
        end if;
      end if;

    end if;
  end process;

  sync_slot_o <= slave_in.addr(log2(NumSyncSlots)-1 downto 0);

end rtl;
