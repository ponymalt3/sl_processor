library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.sl_misc_p.all;

entity sl_dpram is
  
  generic (
    SizeInBytes : natural := 2048;
    SizeOfElementInBits : natural := 32);

  port (
    clk_i      : in    std_ulogic;
    reset_n_i  : in    std_ulogic;

    p0_addr_i : in unsigned(log2(SizeInBytes/((SizeOfElementInBits+7)/8))-1 downto 0);
    p0_din_i : in std_ulogic_vector(SizeOfElementInBits-1 downto 0);
    p0_dout_o : out std_ulogic_vector(SizeOfElementInBits-1 downto 0);
    p0_we_i : in std_ulogic;

    p1_addr_i : in unsigned(log2(SizeInBytes/((SizeOfElementInBits+7)/8))-1 downto 0);
    p1_din_i : in std_ulogic_vector(SizeOfElementInBits-1 downto 0);
    p1_dout_o : out std_ulogic_vector(SizeOfElementInBits-1 downto 0);
    p1_we_i : in std_ulogic);

end entity sl_dpram;

architecture rtl of sl_dpram is

  type mem_t is array (natural range <>) of std_ulogic_vector(SizeOfElementInBits-1 downto 0);
  shared variable mem : mem_t((SizeInBytes)/((SizeOfElementInBits+7)/8)-1 downto 0);

  constant AddrWidth : natural := log2(mem'length);

  signal clk0 : std_ulogic;
  signal clk1 : std_ulogic;

begin  -- architecture rtl

  clk0 <= clk_i;

  process (clk0) is
  begin  -- process
    if clk0'event and clk0 = '1' then  -- rising clock edge
     
     p0_dout_o <= mem(to_integer(p0_addr_i));
     
      if p0_we_i = '1' then
        mem(to_integer(p0_addr_i)) := p0_din_i;
      end if;

    end if;
  end process;

  clk1 <= clk_i;

  process (clk1) is
  begin  -- process
    if clk1'event and clk1 = '1' then  -- rising clock edge
     
      p1_dout_o <= mem(to_integer(p1_addr_i));

      if p1_we_i = '1' then
        mem(to_integer(p1_addr_i)) := p1_din_i;
      end if;
      
    end if;
  end process;

end architecture rtl;
