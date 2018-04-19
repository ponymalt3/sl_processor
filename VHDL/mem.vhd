library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity multi_port_mem is
  
  generic (
    SizeInKBytes : natural := 2);

  port (
    clk_i      : in    std_ulogic;
    reset_n_i  : in    std_ulogic;

    r0_addr_i : in std_ulogic_vector(15 downto 0);
    r0_din_i : in std_ulogic_vector(31 downto 0);
    r0_we_i : in std_ulogic;
    r0_idle_o : out std_ulogic;
    
    f0_addr_i : in std_ulogic_vector(15 downto 0);
    f0_dout_o : out std_ulogic_vector(31 downto 0);
    f0_complete_o : out std_ulogic;

    r1_addr_i : in std_ulogic_vector(15 downto 0);
    r1_din_i : in std_ulogic_vector(31 downto 0);
    r1_dout_o : out std_ulogic_vector(31 downto 0);
    r1_we_i   : in std_ulogic;
    r1_complete_o : out std_ulogic;

    f1_addr_i : in std_ulogic_vector(15 downto 0);
    f1_dout_o : out std_ulogic_vector(31 downto 0);
    f1_complete_o : out std_ulogic);

end entity multi_port_mem;

architecture rtl of multi_port_mem is

  type mem_t is array (natural range <>) of std_ulogic_vector(31 downto 0);

  signal mem : mem_t((SizeInKBytes*8*1024+31)/32-1 downto 0);
  
  signal state : std_ulogic;
  signal mem0_addr : std_ulogic_vector(15 downto 0);
  signal mem1_addr : std_ulogic_vector(15 downto 0);
  signal mem0_dout : std_ulogic_vector(31 downto 0);
  signal mem1_dout : std_ulogic_vector(31 downto 0);

begin  -- architecture rtl

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      state <= '0';
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      state <= not state;

      if r0_we_i = '1' and state = '0' then
        mem(to_integer(unsigned(r0_addr_i))) <= r0_din_i;
      end if;
      
      if r1_we_i = '1' and state = '0' then
        mem(to_integer(unsigned(r1_addr_i))) <= r1_din_i;
      end if;
        
    end if;
  end process;

  mem0_addr <= r0_addr_i when state = '0' else f0_addr_i;
  mem1_addr <= r1_addr_i when state = '0' else f1_addr_i;

  mem0_dout <= mem(to_integer(unsigned(mem0_addr)));
  mem1_dout <= mem(to_integer(unsigned(mem1_addr)));

  f0_dout_o <= mem0_dout;
  r1_dout_o <= mem1_dout;
  f1_dout_o <= mem1_dout;

  r0_idle_o <= not state;
  f0_complete_o <= state;
  r1_complete_o <= not state;
  f1_complete_o <= state;

end architecture rtl;
