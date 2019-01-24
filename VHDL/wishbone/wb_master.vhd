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
    burst_i    : in    unsigned(5 downto 0) := to_unsigned(0,6);
    we_i       : in    std_ulogic;
    dready_o   : out   std_ulogic;
    complete_o : out   std_ulogic;
    err_o      : out   std_ulogic;
    
    master_out_i : in wb_master_ifc_in_t;
    master_out_o : out wb_master_ifc_out_t);

end entity wb_master;

architecture rtl of wb_master is

  type fsm_t is (ST_IDLE,ST_PENDING);
  signal state : fsm_t;

  signal master_out : wb_master_ifc_out_t;
  signal count : unsigned(2 downto 0);
  signal count_ack : unsigned(2 downto 0);
  signal mask : unsigned(2 downto 0);

begin  -- architecture rtl

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      state <= ST_IDLE;
      master_out <= (to_unsigned(0,32),(others => '0'),'0',"0000",'0','0');
      dout_o <= (others => '0');
      complete_o <= '0';
      err_o <= '0';
      dready_o <= '0';
      count <= to_unsigned(0,3);
      mask <= to_unsigned(0,3);
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      complete_o <= '0';
      err_o <= master_out_i.err;
      dready_o <= '0';
      
      if en_i = '1' and state = ST_IDLE then
        state <= ST_PENDING;
        count <= burst_i(2 downto 0);
        count_ack <= burst_i(2 downto 0);
        mask <= burst_i(2 downto 0);
        dready_o <= we_i;
        master_out.adr <= addr_i;
        master_out.dat <= din_i;
        master_out.sel <= (others => '1');
        master_out.we <= we_i;
        master_out.stb <= '1';
        master_out.cyc <= '1';
      elsif state = ST_PENDING then
        if master_out_i.stall = '0' then
          master_out.dat <= din_i;
          master_out.adr(2 downto 0) <= (master_out.adr(2 downto 0) and not mask) or ((master_out.adr(2 downto 0)+1) and mask);
          if count = to_unsigned(0,3) then
            master_out.stb <= '0';
          else
            if master_out.we = '1' then
              dready_o <= '1';
            end if;
            count <= count-1;
          end if;
        end if;

        if master_out_i.ack = '1' then
          count_ack <= count_ack-1;
          if master_out.we = '0' then
            dready_o <= '1';
          end if;
          dout_o <= master_out_i.dat;
        end if;
        
        if (master_out_i.ack = '1' and count_ack = to_unsigned(0,3)) or master_out_i.err = '1' then
          state <= ST_IDLE;
          master_out.stb <= '0';
          master_out.cyc <= '0';
          complete_o <= '1';
          dready_o <= not master_out.we;
        end if;
      end if;
    end if;
  end process;

  master_out_o <= master_out;
  
end architecture rtl;
