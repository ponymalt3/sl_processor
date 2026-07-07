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
    ExtMemSizeInKB    : natural := 8;
    CodeMemSizeInKB   : natural := 8);

  port (
    clk_i          : in std_ulogic;
    mem_clk_i      : in std_ulogic;
    reset_n_i      : in std_ulogic;
    
    core_en_i      : in std_ulogic_vector(3 downto 0);
    core_reset_n_i : in std_ulogic_vector(3 downto 0);

    ext_master_i : in  wb_master_ifc_in_t;
    ext_master_o : out wb_master_ifc_out_t;
    code_master_i : in  wb_master_ifc_in_t;
    code_master_o : out wb_master_ifc_out_t;
    debug_o : out std_ulogic_vector(7 downto 0));

end entity sl_cluster;

architecture rtl of sl_cluster is

  signal master_in : wb_master_ifc_in_array_t(7 downto 0);
  signal master_out : wb_master_ifc_out_array_t(7 downto 0);
  signal slave_in : wb_slave_ifc_in_array_t(1 downto 0);
  signal slave_out : wb_slave_ifc_out_array_t(1 downto 0);

begin  -- architecture rtl

  wb_ixs_1: entity work.wb_ixs
    generic map (
      MasterConfig => (
        wb_master("code_mem"),
        wb_master("ext_mem"),
        wb_master("code_mem"),
        wb_master("ext_mem"),
        wb_master("code_mem"),
        wb_master("ext_mem"),
        wb_master("code_mem"),
        wb_master("ext_mem")),
      SlaveMap     => (
        wb_slave("code_mem",0,(CodeMemSizeInKB*1024)/4),
        wb_slave("ext_mem",(CodeMemSizeInKB*1024)/4,(ExtMemSizeInKB*1024)/4)))

    port map (
      clk_i       => clk_i,
      reset_n_i   => reset_n_i,
      master_in_i => master_out,
      master_in_o => master_in,
      slave_out_i => slave_out,
      slave_out_o => slave_in);

    code_master_o <= slave_in(0);
    slave_out(0) <= code_master_i;
    ext_master_o <= slave_in(1);
    slave_out(1) <= ext_master_i;      

  --debug_o <= code_data(0)(3 downto 0) & To_StdULogicVector(std_logic_vector(code_addr(0)(3 downto 0)));

  proc: for i in 0 to 3 generate
    signal local_reset : std_ulogic_vector(3 downto 0);
    signal code_addr : unsigned(15 downto 0);
    signal code_data : std_ulogic_vector(15 downto 0);
    signal cache_dout : std_ulogic_vector(31 downto 0);
    signal cache_dout_valid : std_ulogic;
    signal code_stall : std_ulogic;
    signal ext_master_out : wb_master_ifc_out_t;
  begin
    local_reset(i) <= reset_n_i and core_reset_n_i(i);
    code_stall <= not cache_dout_valid;

    master_out(2*i+1) <=
      (ext_master_out.adr+to_unsigned((CodeMemSizeInKB*1024)/4,32),
       ext_master_out.dat,
       ext_master_out.we,
       ext_master_out.sel,
       ext_master_out.stb,
       ext_master_out.cyc);

    wb_cache_1: entity work.wb_cache
      generic map (
        WordsPerLine  => 8,
        NumberOfLines => 64,
        WriteTrough   => true)
      port map (
        clk_i           => clk_i,
        mem_clk_i       => mem_clk_i,
        reset_n_i       => reset_n_i,
        addr_i(15 downto 0) => code_addr,
        addr_i(31 downto 16) => X"0000",
        din_i           => (others => '0'),
        dout_o          => cache_dout,
        en_i            => core_en_i(i),
        we_i            => '0',
        complete_o      => cache_dout_valid,
        err_o           => open,
        snooping_addr_i => to_unsigned(0,32),
        snooping_en_i   => '0',
        master_out_i    => master_in(2*i),
        master_out_o    => master_out(2*i));
    
    sl_processor_1: entity work.sl_processor
      generic map (
        LocalMemSizeInKB => LocalMemSizeInKB,
        CodeStartAddr  => i*4)
      port map (
        clk_i           => clk_i,
        mem_clk_i       => mem_clk_i,
        reset_n_i       => reset_n_i,
        core_en_i       => core_en_i(i),
        core_reset_n_i  => local_reset(i),
        code_addr_o     => code_addr,
        code_stall_i    => code_stall,
        code_data_i     => cache_dout(15 downto 0),
        ext_master_i    => master_in(2*i+1),
        ext_master_o    => ext_master_out,
        debug_slave_i   => (to_unsigned(0,32),(others => '0'),'0',(others => '0'),'0','0'),
        debug_slave_o   => open,
        executed_addr_o => open);
  end generate proc;

end architecture rtl;
