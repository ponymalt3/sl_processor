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
    LocalMemSizeInKB   : natural := 2;
    ExtMemSizeInKB     : natural := 16*1024;
    CodeMemSizeInKB    : natural := 2;
    CodeCacheSizeInKB  : natural := 2;
    DataCacheSizeInKB  : natural := 2;
    PeripheralSizeInKB : natural := 8);

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

    snoop_addr_i : in unsigned(31 downto 0) := to_unsigned(0,32);
    snoop_en_i   : in std_ulogic := '0');

end entity sl_cluster;

architecture rtl of sl_cluster is

  signal master_in : wb_master_ifc_in_array_t(7 downto 0);
  signal master_out : wb_master_ifc_out_array_t(7 downto 0);
  signal slave_in : wb_slave_ifc_in_array_t(1 downto 0);
  signal slave_out : wb_slave_ifc_out_array_t(1 downto 0);

  constant CacheWordsPerLine : natural := 8;
  constant CodeCacheLines : natural := (CodeCacheSizeInKB*1024)/(CacheWordsPerLine*4);
  constant DataCacheLines : natural := (DataCacheSizeInKB*1024)/(CacheWordsPerLine*4);

  constant CodeMemSize : natural := (CodeMemSizeInKB*1024)/4;
  constant ExtMemSize : natural := (ExtMemSizeInKB*1024)/4;
  constant PeripheralSize : natural := (PeripheralSizeInKB*1024)/4;

  signal snoop_in_code_range : std_ulogic;
  signal data_snoop_addr : unsigned(31 downto 0);

begin  -- architecture rtl

  wb_ixs_1: entity work.wb_ixs
    generic map (
      MasterConfig => (
        wb_master("code_mem"),
        wb_master("code_mem"),
        wb_master("code_mem"),
        wb_master("code_mem"),
        wb_master("ext_mem"),
        wb_master("ext_mem"),
        wb_master("ext_mem"),
        wb_master("ext_mem")),
      SlaveMap     => (
        wb_slave("code_mem",0,CodeMemSize),
        wb_slave("ext_mem",0,ExtMemSize-CodeMemSize+PeripheralSize)))

    port map (
      clk_i       => clk_i,
      reset_n_i   => reset_n_i,
      master_in_i => master_out,
      master_in_o => master_in,
      slave_out_i => slave_out,
      slave_out_o => slave_in);

    code_master_o <= slave_in(0);
    slave_out(0) <= code_master_i;

    -- offsets ext mem
    ext_master_o <=
      (slave_in(1).adr+to_unsigned(CodeMemSize,32),
       slave_in(1).dat,
       slave_in(1).we,
       slave_in(1).sel,
       slave_in(1).stb,
       slave_in(1).cyc);
    slave_out(1) <= ext_master_i;

  proc: for i in 0 to 3 generate
    signal local_reset : std_ulogic_vector(3 downto 0);
    signal code_addr : unsigned(15 downto 0);
    signal code_data : std_ulogic_vector(15 downto 0);
    signal code_stall : std_ulogic;
    signal code_cache_in  : wb_slave_ifc_in_t;
    signal code_cache_out : wb_slave_ifc_out_t;
    signal ext_master_out : wb_master_ifc_out_t;
    signal ext_master_in : wb_master_ifc_in_t;

    -- code_cache_1's flat request/ack side (no settle delay: core_en_i(i)
    -- and code_addr are the core's own directly-wired combinational
    -- signals, never routed through wb_ixs_arbiter.vhd)
    signal code_cache_addr     : unsigned(31 downto 0);
    signal code_cache_wdata    : std_ulogic_vector(31 downto 0);
    signal code_cache_rdata    : std_ulogic_vector(31 downto 0);
    signal code_cache_en       : std_ulogic;
    signal code_cache_we       : std_ulogic;
    signal code_cache_complete : std_ulogic;
    signal code_cache_err      : std_ulogic;

    -- data_cache_1's flat request/ack side (same: sl_processor's
    -- ext_master_o is wired directly, not through wb_ixs_arbiter.vhd)
    signal data_cache_addr     : unsigned(31 downto 0);
    signal data_cache_wdata    : std_ulogic_vector(31 downto 0);
    signal data_cache_rdata    : std_ulogic_vector(31 downto 0);
    signal data_cache_en       : std_ulogic;
    signal data_cache_we       : std_ulogic;
    signal data_cache_complete : std_ulogic;
    signal data_cache_err      : std_ulogic;
  begin
    local_reset(i) <= reset_n_i and core_reset_n_i(i);
    code_stall <= not code_cache_out.ack;
    code_cache_in.adr(14 downto 0)  <= code_addr(15 downto 1);
    code_cache_in.adr(31 downto 15) <= (others => '0');
    code_cache_in.dat <= (others => '0');
    code_cache_in.we  <= '0';
    code_cache_in.sel <= (others => '1');
    code_cache_in.stb <= core_en_i(i);
    code_cache_in.cyc <= core_en_i(i);

    -- code_mem is word-addressed (2 instructions/word); low bit of the
    -- halfword PC selects which half of the fetched word is the instruction
    code_data <= code_cache_out.dat(31 downto 16) when code_addr(0) = '1' else code_cache_out.dat(15 downto 0);

    wb_cache_adapter_code: entity work.wb_cache_adapter
      generic map (IsConnectedToIXS => false)
      port map (
        clk_i      => clk_i,
        reset_n_i  => reset_n_i,
        addr_o     => code_cache_addr,
        din_i      => code_cache_rdata,
        dout_o     => code_cache_wdata,
        en_o       => code_cache_en,
        we_o       => code_cache_we,
        complete_i => code_cache_complete,
        err_i      => code_cache_err,
        slave_i    => code_cache_in,
        slave_o    => code_cache_out);

    wb_cache_1: entity work.wb_cache
      generic map (
        WordsPerLine  => CacheWordsPerLine,
        NumberOfLines => CodeCacheLines,
        WriteThrough  => true,
        NarrowTag     => true)
      port map (
        clk_i           => clk_i,
        mem_clk_i       => mem_clk_i,
        reset_n_i       => reset_n_i,
        addr_i          => code_cache_addr,
        din_i           => code_cache_wdata,
        dout_o          => code_cache_rdata,
        en_i            => code_cache_en,
        we_i            => code_cache_we,
        complete_o      => code_cache_complete,
        err_o           => code_cache_err,
        snooping_addr_i => snoop_addr_i,
        snooping_en_i   => snoop_en_i,
        master_out_i    => master_in(i),
        master_out_o    => master_out(i));

    wb_cache_adapter_data: entity work.wb_cache_adapter
      generic map (IsConnectedToIXS => false)
      port map (
        clk_i      => clk_i,
        reset_n_i  => reset_n_i,
        addr_o     => data_cache_addr,
        din_i      => data_cache_rdata,
        dout_o     => data_cache_wdata,
        en_o       => data_cache_en,
        we_o       => data_cache_we,
        complete_i => data_cache_complete,
        err_i      => data_cache_err,
        slave_i    => ext_master_out,
        slave_o    => ext_master_in);

    data_cache_1: entity work.wb_cache
      generic map (
        WordsPerLine   => CacheWordsPerLine,
        NumberOfLines  => DataCacheLines,
        WriteThrough   => true,
        NarrowTag      => true,
        EnableBypass   => true,
        BypassBaseAddr => CacheWordsPerLine*65536)  -- 2**(WordIndexBits+16) words = NarrowTag's addressable cap
      port map (
        clk_i           => clk_i,
        mem_clk_i       => mem_clk_i,
        reset_n_i       => reset_n_i,
        addr_i          => data_cache_addr,
        din_i           => data_cache_wdata,
        dout_o          => data_cache_rdata,
        en_i            => data_cache_en,
        we_i            => data_cache_we,
        complete_o      => data_cache_complete,
        err_o           => data_cache_err,
        snooping_addr_i => snoop_addr_i-to_unsigned(CodeMemSize, 32),
        snooping_en_i   => snoop_en_i,
        master_out_i    => master_in(4+i),
        master_out_o    => master_out(4+i));

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
        code_data_i     => code_data,
        ext_master_i    => ext_master_in,
        ext_master_o    => ext_master_out,
        debug_slave_i   => (to_unsigned(0,32),(others => '0'),'0',(others => '0'),'0','0'),
        debug_slave_o   => open,
        executed_addr_o => open);
  end generate proc;

end architecture rtl;
