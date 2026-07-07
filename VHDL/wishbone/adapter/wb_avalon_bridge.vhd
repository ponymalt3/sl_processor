library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;

entity wb_avalon_bridge is
  
  port (
    clk_i      : in    std_ulogic;
    reset_n_i  : in    std_ulogic;
    
    slave_i : in wb_slave_ifc_in_t;
    slave_o : out wb_slave_ifc_out_t;
    
    avalon_address_o       : out    std_logic_vector(23 downto 0);
    avalon_byteenable_n_o  : out    std_logic_vector(1 downto 0);
    avalon_chipselect_o    : out    std_logic;
    avalon_writedata_o     : out    std_logic_vector(15 downto 0);
    avalon_read_n_o        : out    std_logic;
    avalon_write_n_o       : out    std_logic;
    avalon_readdata_i      : in   std_logic_vector(15 downto 0);
    avalon_readdatavalid_i : in   std_logic;
    avalon_waitrequest_i   : in   std_logic);

end entity wb_avalon_bridge;

architecture rtl of wb_avalon_bridge is

  signal state : std_ulogic;
  signal rstate : std_ulogic;
  signal avalon_din : std_ulogic_vector(31 downto 0);
  signal avalon_dready : std_ulogic;
  signal avalon_read_n : std_ulogic;
  signal avalon_write_n : std_ulogic;
  signal stall : std_ulogic;
  signal dready : std_ulogic;

begin  -- architecture rtl

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      state <= '0';
      rstate <= '0';
      avalon_din <= (others => '0');
      avalon_dready <= '0';      
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      avalon_dready <= '0';

      if (avalon_write_n = '0' or avalon_read_n = '0') and avalon_waitrequest_i = '0' then
        state <= not state;
      end if;
      
      if avalon_readdatavalid_i = '1' then
        avalon_din <= avalon_din(15 downto 0) & to_stdULogicVector(avalon_readdata_i);
        rstate <= not rstate;
        avalon_dready <= rstate;
      end if;
    end if;
  end process;

  avalon_address_o <= std_logic_vector(slave_i.adr(22 downto 0)) & state;
  avalon_byteenable_n_o <= "00";
  avalon_chipselect_o <= '1';
  avalon_writedata_o <= std_logic_vector(slave_i.dat(15 downto 0)) when state = '0' else std_logic_vector(slave_i.dat(31 downto 16));
  avalon_write_n <= not(slave_i.cyc and slave_i.stb and slave_i.we);
  avalon_read_n <= not(slave_i.cyc and slave_i.stb and not slave_i.we);
  avalon_write_n_o <= avalon_write_n;
  avalon_read_n_o <= avalon_read_n;

  stall <= '1' when state = '0' or avalon_waitrequest_i = '1' else '0';
  dready <= avalon_dready or (state and not avalon_write_n and not avalon_waitrequest_i); 

  slave_o <= (avalon_din,dready,'0',stall);           
    
end architecture rtl;
