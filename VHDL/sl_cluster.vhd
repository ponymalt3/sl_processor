library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.std_logic_textio.all; -- read std_ulogic etc

library std;
use std.textio.all;

use work.sl_misc_p.all;
use work.wishbone_p.all;

entity sl_cluster is
  
  generic (
    LocalMemSizeInKB  : natural := 2;
    SharedMemSizeInKB : natural := 8;
    CodeMemSizeInKB   : natural := 8);

  port (
    clk_i          : in std_ulogic;
    reset_n_i      : in std_ulogic;
    
    core_en_i      : in std_ulogic_vector(3 downto 0);
    core_reset_n_i : in std_ulogic_vector(3 downto 0);

    master_i : in  wb_master_ifc_in_t;
    master_o : out wb_master_ifc_out_t;
    slave_i : in  wb_slave_ifc_in_t;
    slave_o : out wb_slave_ifc_out_t);

end entity sl_cluster;

architecture rtl of sl_cluster is

  type code_addr_array_t is array (natural range <>) of unsigned(15 downto 0);
  type code_data_array_t is array (natural range <>) of std_ulogic_vector(15 downto 0);
  signal code_addr : code_addr_array_t(3 downto 0);
  signal code_re : std_ulogic_vector(3 downto 0);  
  signal code_data : code_data_array_t(3 downto 0);

  signal master_in : wb_master_ifc_in_array_t(4 downto 0);
  signal master_out : wb_master_ifc_out_array_t(4 downto 0);
  signal slave_in : wb_slave_ifc_in_array_t(6 downto 0);
  signal slave_out : wb_slave_ifc_out_array_t(6 downto 0);

  signal shared_mem_dout : std_ulogic_vector(31 downto 0);
  signal shared_mem_we : std_ulogic;
  signal shared_mem_addr : std_ulogic_vector(15 downto 0);

  signal ack : std_ulogic;

  type mem_t is array (natural range <>) of std_logic_vector(31 downto 0);
  signal mem : mem_t((SharedMemSizeInKB*1024/4)-1 downto 0);

begin  -- architecture rtl

  wb_ixs_1: entity work.wb_ixs
    generic map (
      MasterConfig => (
        wb_master("shared_mem ext_mem"),
        wb_master("shared_mem ext_mem"),
        wb_master("shared_mem ext_mem"),
        wb_master("shared_mem ext_mem"),
        wb_master("shared_mem core0_mem core1_mem core2_mem core3_mem code_mem")),
      SlaveMap     => (
        wb_slave("shared_mem",0,1024/4*SharedMemSizeInKB),
        wb_slave("core0_mem",1024/4*(SharedMemSizeInKB+0*LocalMemSizeInKB),1024/4*LocalMemSizeInKB),
        wb_slave("core1_mem",1024/4*(SharedMemSizeInKB+1*LocalMemSizeInKB),1024/4*LocalMemSizeInKB),
        wb_slave("core2_mem",1024/4*(SharedMemSizeInKB+2*LocalMemSizeInKB),1024/4*LocalMemSizeInKB),
        wb_slave("core3_mem",1024/4*(SharedMemSizeInKB+3*LocalMemSizeInKB),1024/4*LocalMemSizeInKB),
        wb_slave("ext_mem",1024/4*(SharedMemSizeInKB+4*LocalMemSizeInKB),64/4*1024),
        wb_slave("code_mem",1024/4*(SharedMemSizeInKB+4*LocalMemSizeInKB+64),1024/2*CodeMemSizeInKB)))

    port map (
      clk_i       => clk_i,
      reset_n_i   => reset_n_i,
      master_in_i => master_out,
      master_in_o => master_in,
      slave_out_i => slave_out,
      slave_out_o => slave_in);

  slave_o <= master_in(4);
  master_out(4) <= slave_i;

  slave_out(5) <= master_i;
  master_o <= slave_in(5);

  sl_code_mem_1: entity work.sl_code_mem
    generic map (
      SizeInKBytes => CodeMemSizeInKB)
    port map (
      clk_i        => clk_i,
      reset_n_i    => reset_n_i,
      p0_addr_i    => code_addr(0),
      p0_code_o    => code_data(0),
      p0_re_i      => code_re(0),
      p0_reset_n_i => core_reset_n_i(0),
      p1_addr_i    => code_addr(1),
      p1_code_o    => code_data(1),
      p1_re_i      => code_re(1),
      p1_reset_n_i => core_reset_n_i(1),
      p2_addr_i    => code_addr(2),
      p2_code_o    => code_data(2),
      p2_re_i      => code_re(2),
      p2_reset_n_i => core_reset_n_i(2),
      p3_addr_i    => code_addr(3),
      p3_code_o    => code_data(3),
      p3_re_i      => code_re(3),
      p3_reset_n_i => core_reset_n_i(3),
      wb_slave_i   => slave_in(6),
      wb_slave_o   => slave_out(6));

  proc: for i in 0 to 3 generate
    signal local_reset : std_ulogic_vector(3 downto 0);
  begin
    local_reset(i) <= reset_n_i and core_reset_n_i(i);
    
    sl_processor_1: entity work.sl_processor
      generic map (
        LocalMemSizeInKB => LocalMemSizeInKB,
        UseCodeAddrNext  => i<2)
      port map (
        clk_i           => clk_i,
        reset_n_i       => reset_n_i,
        core_en_i       => core_en_i(i),
        core_reset_n_i  => local_reset(i),
        code_addr_o     => code_addr(i),
        code_re_o       => code_re(i),
        code_data_i     => code_data(i),
        ext_master_i    => master_in(i),
        ext_master_o    => master_out(i),
        mem_slave_i     => slave_in(1+i),
        mem_slave_o     => slave_out(1+i),
        executed_addr_o => open);
  end generate proc;

  -- shared mem
  m9k_1: entity work.m9k
    generic map (
      SizeInKBytes        => SharedMemSizeInKB,
      SizeOfElementInBits => 32)
    port map (
      clk_i     => clk_i,
      reset_n_i => reset_n_i,
      p0_en_i   => '1',
      p0_addr_i => shared_mem_addr,
      p0_din_i  => slave_in(0).dat,
      p0_dout_o => open,--shared_mem_dout,
      p0_we_i   => shared_mem_we,
      p1_en_i   => '0',
      p1_addr_i => X"0000",
      p1_din_i  => (others => '0'),
      p1_dout_o => open,
      p1_we_i   => '0');

  shared_mem_addr <= To_StdULogicVector(std_logic_vector(slave_in(0).adr(15 downto 0)));
  process (clk_i) is
  begin  -- process
    if clk_i'event and clk_i = '1' then  -- rising clock edge
     
     shared_mem_dout <= To_StdULogicVector(mem(to_integer(slave_in(0).adr(15 downto 0))));
     
      if shared_mem_we = '1' then
        mem(to_integer(slave_in(0).adr(15 downto 0))) <= std_logic_vector(slave_in(0).dat);
      end if;

    end if;
  end process;

  process (clk_i, reset_n_i) is
  begin  -- process
    if reset_n_i = '0' then             -- asynchronous reset (active low)
      ack <= '0';
    elsif clk_i'event and clk_i = '1' then  -- rising clock edge
      ack <= slave_in(0).stb and slave_in(0).cyc;
    end if;
  end process;

  slave_out(0) <= (shared_mem_dout,ack,'0','0');
  shared_mem_we <= '1' when slave_in(0).stb = '1' and slave_in(0).cyc = '1' and slave_in(0).we = '1' else '0'; 

end architecture rtl;
