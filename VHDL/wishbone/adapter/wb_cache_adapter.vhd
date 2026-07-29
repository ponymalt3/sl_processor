library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;
use work.sl_misc_p.all;

entity wb_cache_adapter is
  generic (
    IsConnectedToIXS : boolean := false);
  port (
    clk_i      : in  std_ulogic;
    reset_n_i  : in  std_ulogic;

    addr_o     : out unsigned(31 downto 0);
    din_i      : in  std_ulogic_vector(31 downto 0);
    dout_o     : out std_ulogic_vector(31 downto 0);
    en_o       : out std_ulogic;
    we_o       : out std_ulogic;
    complete_i : in  std_ulogic;
    err_i      : in  std_ulogic;

    slave_i : in  wb_slave_ifc_in_t;
    slave_o : out wb_slave_ifc_out_t);

end entity wb_cache_adapter;

architecture rtl of wb_cache_adapter is

  signal req     : std_ulogic;
  signal req_1d  : std_ulogic;
  signal stall   : std_ulogic;

begin

  req <= slave_i.stb and slave_i.cyc;

  -- IXS using falling edge (same as cache) => need delay
  process (clk_i, reset_n_i) is
  begin
    if reset_n_i = '0' then
      req_1d <= '0';
    elsif clk_i'event and clk_i = '1' then
      req_1d <= req;
    end if;
  end process;

  stall <= ((req and not req_1d) or not complete_i) when IsConnectedToIXS else not complete_i;

  addr_o  <= slave_i.adr;
  dout_o  <= slave_i.dat;
  we_o    <= slave_i.we;
  en_o    <= (req and req_1d) when IsConnectedToIXS else req;
  slave_o <= (din_i, complete_i, err_i, stall);

end architecture rtl;
