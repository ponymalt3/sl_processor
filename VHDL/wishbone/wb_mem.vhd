library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;
use work.sl_misc_p.all;

entity wb_mem is
  
  generic (
    MemSizeInKB : natural := 2);

  port (
    clk_i : in std_ulogic;
    mem_clk_i : in std_ulogic;
    reset_n_i : in std_ulogic;
    
    slave0_i : in wb_slave_ifc_in_t;
    slave0_o : out wb_slave_ifc_out_t;    
    slave1_i : in wb_slave_ifc_in_t := (to_unsigned(0,32),(others => 'Z'),'0',(others => '0'),'0','0');
    slave1_o : out wb_slave_ifc_out_t);

end entity wb_mem;

architecture rtl of wb_mem is

  constant AddrWidth : natural := log2(MemSizeInKB*1024/4);

  signal slave0_in : wb_slave_ifc_in_t;
  signal slave1_in : wb_slave_ifc_in_t;
  signal mem_dout0 : std_ulogic_vector(31 downto 0);
  signal mem_dout1 : std_ulogic_vector(31 downto 0);

begin  -- architecture rtl

  sl_dpram_1: entity work.sl_dpram
    generic map (
      SizeInBytes         => MemSizeInKB*1024,
      SizeOfElementInBits => 32)
    port map (
      clk_i     => mem_clk_i,
      reset_n_i => reset_n_i,
      p0_addr_i => slave0_in.adr(AddrWidth-1 downto 0),
      p0_din_i  => slave0_in.dat,
      p0_dout_o => mem_dout0,
      p0_we_i   => slave0_in.we,
      p1_addr_i => slave1_in.adr(AddrWidth-1 downto 0),
      p1_din_i  => slave1_in.dat,
      p1_dout_o => mem_dout1,
      p1_we_i   => slave1_in.we);

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      slave0_in <= (to_unsigned(0,32),(others => '0'),'0',(others => '0'),'0','0');
      slave1_in <= (to_unsigned(0,32),(others => '0'),'0',(others => '0'),'0','0');
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      slave0_in <= slave0_i;
      slave1_in <= slave1_i;
    end if;
  end process;

  slave0_o <= (mem_dout0,slave0_in.cyc and slave0_in.stb,'0','0');
  slave1_o <= (mem_dout1,slave1_in.cyc and slave1_in.stb,'0','0');

end architecture rtl;
