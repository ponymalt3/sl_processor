library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.sl_misc_p.all;

entity m9k is
  
  generic (
    SizeInKBytes : natural := 2;
    SizeOfElementInBits : natural := 32);

  port (
    clk_i      : in    std_ulogic;
    reset_n_i  : in    std_ulogic;

    p0_en_i : in std_ulogic;
    p0_addr_i : in std_ulogic_vector(15 downto 0);
    p0_din_i : in std_ulogic_vector(SizeOfElementInBits-1 downto 0);
    p0_dout_o : out std_ulogic_vector(SizeOfElementInBits-1 downto 0);
    p0_we_i : in std_ulogic;

    p1_en_i : in std_ulogic;
    p1_addr_i : in std_ulogic_vector(15 downto 0);
    p1_din_i : in std_ulogic_vector(SizeOfElementInBits-1 downto 0);
    p1_dout_o : out std_ulogic_vector(SizeOfElementInBits-1 downto 0);
    p1_we_i : in std_ulogic);

end entity m9k;

architecture rtl of m9k is

  type mem_t is array (natural range <>) of std_ulogic_vector(SizeOfElementInBits-1 downto 0);
  shared variable mem : mem_t((SizeInKBytes*1024)/((SizeOfElementInBits+7)/8)-1 downto 0);

  constant AddrWidth : natural := log2(mem'length);

  signal clk_gate_0 : std_ulogic;
  signal clk0 : std_ulogic;
  signal clk_gate_1 : std_ulogic;
  signal clk1 : std_ulogic;

begin  -- architecture rtl

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      clk_gate_0 <= '1';
      clk_gate_1 <= '1';
    elsif clk_i'event and clk_i = '0' then  -- rising clock edge
      clk_gate_0 <= p0_en_i;
      clk_gate_1 <= p1_en_i;
    end if;
  end process;

  clk0 <= clk_i and clk_gate_0;

  process (clk0) is
  begin  -- process
    if clk0'event and clk0 = '1' then  -- rising clock edge
     
     p0_dout_o <= mem(to_integer(unsigned(p0_addr_i(AddrWidth-1 downto 0))));
     
      if p0_we_i = '1' then
        mem(to_integer(unsigned(p0_addr_i(AddrWidth-1 downto 0)))) := p0_din_i;
      end if;

    end if;
  end process;

  clk1 <= clk_i and clk_gate_1;

  process (clk1) is
  begin  -- process
    if clk1'event and clk1 = '1' then  -- rising clock edge
     
      p1_dout_o <= mem(to_integer(unsigned(p1_addr_i(AddrWidth-1 downto 0))));

      if p1_we_i = '1' then
        mem(to_integer(unsigned(p1_addr_i(AddrWidth-1 downto 0)))) := p1_din_i;
      end if;
      
    end if;
  end process;

end architecture rtl;
