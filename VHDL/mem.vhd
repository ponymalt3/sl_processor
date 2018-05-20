library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity multi_port_mem is
  
  generic (
    SizeInKBytes : natural := 2);

  port (
    clk_i      : in    std_ulogic;
    reset_n_i  : in    std_ulogic;

    wport_addr_i : in std_ulogic_vector(15 downto 0);
    wport_din_i : in std_ulogic_vector(31 downto 0);
    wport_we_i : in std_ulogic;
    
    rport0_addr_i : in std_ulogic_vector(15 downto 0);
    rport0_dout_o : out std_ulogic_vector(31 downto 0);
    rport0_en_i : in std_ulogic;

    rwport_addr_i : in std_ulogic_vector(15 downto 0);
    rwport_din_i : in std_ulogic_vector(31 downto 0);
    rwport_dout_o : out std_ulogic_vector(31 downto 0);
    rwport_we_i   : in std_ulogic;
    rwport_complete_o : out std_ulogic;

    rport1_addr_i : in std_ulogic_vector(15 downto 0);
    rport1_dout_o : out std_ulogic_vector(31 downto 0);

    rport_stall_i : in std_ulogic);

end entity multi_port_mem;

architecture rtl of multi_port_mem is

  signal p0_addr : std_ulogic_vector(15 downto 0);
  signal p0_din  : std_ulogic_vector(31 downto 0);
  signal p0_dout : std_ulogic_vector(31 downto 0);
  signal p0_we   : std_ulogic;
  signal p1_addr : std_ulogic_vector(15 downto 0);
  signal p1_din  : std_ulogic_vector(31 downto 0);
  signal p1_dout : std_ulogic_vector(31 downto 0);
  signal p1_we   : std_ulogic;

  signal p0_en : std_ulogic;
  signal p1_en : std_ulogic;
  
  signal state : std_ulogic;
  
begin  -- architecture rtl

  m9k_1: entity work.m9k
    generic map (
      SizeInKBytes => SizeInKBytes,
      SizeOfElementInBits => 32)
    port map (
      clk_i     => clk_i,
      reset_n_i => reset_n_i,
      p0_en_i   => p0_en,
      p0_addr_i => p0_addr,
      p0_din_i  => p0_din,
      p0_dout_o => p0_dout,
      p0_we_i   => p0_we,
      p1_en_i   => p1_en,
      p1_addr_i => p1_addr,
      p1_din_i  => p1_din,
      p1_dout_o => p1_dout,
      p1_we_i   => p1_we);

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      state <= '0';
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      state <= not state;        
    end if;
  end process;

  p0_addr <= rwport_addr_i when state = '0' else rport0_addr_i;
  p1_addr <= wport_addr_i when state = '0' else rport1_addr_i;

  p0_we <= rwport_we_i and not state;
  p1_we <= wport_we_i and not state;

  p0_din <= rwport_din_i;
  p1_din <= wport_din_i;

  p0_en <= not rport_stall_i;
  p1_en <= not rport_stall_i;

  rport0_dout_o <= p0_dout;
  rport1_dout_o <= p1_dout;
  rwport_dout_o <= p0_dout;

  rwport_complete_o <= state and (rwport_we_i or not rport0_en_i);

end architecture rtl;
