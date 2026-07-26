-- Flat-port wrapper around wb_cache for cocotb.
-- Instantiates both a write-back and a write-through cache variant with
-- generics fixed and Wishbone records unpacked to individual signals so
-- cocotb/GHDL-VPI can drive and read every port without record-access issues.
-- mem_clk is generated here as NOT clk so the test side only sees one clock.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;

entity wb_cache_wrapper is
  port (
    clk_i     : in std_ulogic;
    reset_n_i : in std_ulogic;

    -- -----------------------------------------------------------------------
    -- Write-back cache (wb_*)
    -- -----------------------------------------------------------------------
    wb_addr_i     : in  unsigned(31 downto 0);
    wb_din_i      : in  std_ulogic_vector(31 downto 0);
    wb_dout_o     : out std_ulogic_vector(31 downto 0);
    wb_en_i       : in  std_ulogic;
    wb_we_i       : in  std_ulogic;
    wb_complete_o : out std_ulogic;

    -- Wishbone master response (driven by Python memory model)
    wb_m_dat_i   : in  std_ulogic_vector(31 downto 0);
    wb_m_ack_i   : in  std_ulogic;
    wb_m_err_i   : in  std_ulogic;
    wb_m_stall_i : in  std_ulogic;

    -- Wishbone master request (read by Python memory model)
    wb_m_adr_o : out unsigned(31 downto 0);
    wb_m_dat_o : out std_ulogic_vector(31 downto 0);
    wb_m_we_o  : out std_ulogic;
    wb_m_stb_o : out std_ulogic;
    wb_m_cyc_o : out std_ulogic;

    -- -----------------------------------------------------------------------
    -- Write-through cache (wt_*)
    -- -----------------------------------------------------------------------
    wt_addr_i     : in  unsigned(31 downto 0);
    wt_din_i      : in  std_ulogic_vector(31 downto 0);
    wt_dout_o     : out std_ulogic_vector(31 downto 0);
    wt_en_i       : in  std_ulogic;
    wt_we_i       : in  std_ulogic;
    wt_complete_o : out std_ulogic;

    -- Snooping / invalidation
    wt_inv_addr_i : in  unsigned(31 downto 0);
    wt_inv_en_i   : in  std_ulogic;

    -- Wishbone master response (driven by Python memory model)
    wt_m_dat_i   : in  std_ulogic_vector(31 downto 0);
    wt_m_ack_i   : in  std_ulogic;
    wt_m_err_i   : in  std_ulogic;
    wt_m_stall_i : in  std_ulogic;

    -- Wishbone master request (read by Python memory model)
    wt_m_adr_o : out unsigned(31 downto 0);
    wt_m_dat_o : out std_ulogic_vector(31 downto 0);
    wt_m_we_o  : out std_ulogic;
    wt_m_stb_o : out std_ulogic;
    wt_m_cyc_o : out std_ulogic;

    -- -----------------------------------------------------------------------
    -- Write-through cache with non-cacheable passthrough enabled (byp_*) --
    -- mirrors sl_cluster.vhd's data-cache instantiation (WriteThrough +
    -- EnableBypass together). BypassBaseAddr=48 sits inside the cache's own
    -- 64-word indexable range [0,64) so both "normal cacheable" and
    -- "bypassed despite being in-range" addresses are exercised.
    -- -----------------------------------------------------------------------
    byp_addr_i     : in  unsigned(31 downto 0);
    byp_din_i      : in  std_ulogic_vector(31 downto 0);
    byp_dout_o     : out std_ulogic_vector(31 downto 0);
    byp_en_i       : in  std_ulogic;
    byp_we_i       : in  std_ulogic;
    byp_complete_o : out std_ulogic;

    byp_m_dat_i   : in  std_ulogic_vector(31 downto 0);
    byp_m_ack_i   : in  std_ulogic;
    byp_m_err_i   : in  std_ulogic;
    byp_m_stall_i : in  std_ulogic;

    byp_m_adr_o : out unsigned(31 downto 0);
    byp_m_dat_o : out std_ulogic_vector(31 downto 0);
    byp_m_we_o  : out std_ulogic;
    byp_m_stb_o : out std_ulogic;
    byp_m_cyc_o : out std_ulogic
  );
end entity wb_cache_wrapper;

architecture rtl of wb_cache_wrapper is

  signal mem_clk : std_ulogic;

  signal wb_m_in  : wb_master_ifc_in_t;
  signal wb_m_out : wb_master_ifc_out_t;
  signal wt_m_in  : wb_master_ifc_in_t;
  signal wt_m_out : wb_master_ifc_out_t;
  signal byp_m_in  : wb_master_ifc_in_t;
  signal byp_m_out : wb_master_ifc_out_t;

  -- wb_cache is wishbone-native now; build slave_i/slave_o records from the
  -- flat ports (cyc/stb both just track en_i, matching this wrapper's old
  -- flat en_i semantics -- no held-cyc/bus-lock exercised here yet)
  signal wb_slave_in   : wb_slave_ifc_in_t;
  signal wb_slave_out  : wb_slave_ifc_out_t;
  signal wt_slave_in   : wb_slave_ifc_in_t;
  signal wt_slave_out  : wb_slave_ifc_out_t;
  signal byp_slave_in  : wb_slave_ifc_in_t;
  signal byp_slave_out : wb_slave_ifc_out_t;

begin

  wb_slave_in.adr <= wb_addr_i;
  wb_slave_in.dat <= wb_din_i;
  wb_slave_in.we  <= wb_we_i;
  wb_slave_in.sel <= (others => '1');
  wb_slave_in.stb <= wb_en_i;
  wb_slave_in.cyc <= wb_en_i;
  wb_dout_o     <= wb_slave_out.dat;
  wb_complete_o <= wb_slave_out.ack;

  wt_slave_in.adr <= wt_addr_i;
  wt_slave_in.dat <= wt_din_i;
  wt_slave_in.we  <= wt_we_i;
  wt_slave_in.sel <= (others => '1');
  wt_slave_in.stb <= wt_en_i;
  wt_slave_in.cyc <= wt_en_i;
  wt_dout_o     <= wt_slave_out.dat;
  wt_complete_o <= wt_slave_out.ack;

  byp_slave_in.adr <= byp_addr_i;
  byp_slave_in.dat <= byp_din_i;
  byp_slave_in.we  <= byp_we_i;
  byp_slave_in.sel <= (others => '1');
  byp_slave_in.stb <= byp_en_i;
  byp_slave_in.cyc <= byp_en_i;
  byp_dout_o     <= byp_slave_out.dat;
  byp_complete_o <= byp_slave_out.ack;

  mem_clk <= not clk_i;

  -- Unpack flat ports into Wishbone records
  wb_m_in.dat   <= wb_m_dat_i;
  wb_m_in.ack   <= wb_m_ack_i;
  wb_m_in.err   <= wb_m_err_i;
  wb_m_in.stall <= wb_m_stall_i;
  wb_m_adr_o    <= wb_m_out.adr;
  wb_m_dat_o    <= wb_m_out.dat;
  wb_m_we_o     <= wb_m_out.we;
  wb_m_stb_o    <= wb_m_out.stb;
  wb_m_cyc_o    <= wb_m_out.cyc;

  wt_m_in.dat   <= wt_m_dat_i;
  wt_m_in.ack   <= wt_m_ack_i;
  wt_m_in.err   <= wt_m_err_i;
  wt_m_in.stall <= wt_m_stall_i;
  wt_m_adr_o    <= wt_m_out.adr;
  wt_m_dat_o    <= wt_m_out.dat;
  wt_m_we_o     <= wt_m_out.we;
  wt_m_stb_o    <= wt_m_out.stb;
  wt_m_cyc_o    <= wt_m_out.cyc;

  byp_m_in.dat   <= byp_m_dat_i;
  byp_m_in.ack   <= byp_m_ack_i;
  byp_m_in.err   <= byp_m_err_i;
  byp_m_in.stall <= byp_m_stall_i;
  byp_m_adr_o    <= byp_m_out.adr;
  byp_m_dat_o    <= byp_m_out.dat;
  byp_m_we_o     <= byp_m_out.we;
  byp_m_stb_o    <= byp_m_out.stb;
  byp_m_cyc_o    <= byp_m_out.cyc;

  DUT_WB : entity work.wb_cache
    generic map (WordsPerLine => 4, NumberOfLines => 16, WriteThrough => false)
    port map (
      clk_i           => clk_i,
      mem_clk_i       => mem_clk,
      reset_n_i       => reset_n_i,
      slave_i         => wb_slave_in,
      slave_o         => wb_slave_out,
      snooping_addr_i => to_unsigned(0, 32),
      snooping_en_i   => '0',
      master_out_i    => wb_m_in,
      master_out_o    => wb_m_out);

  DUT_WT : entity work.wb_cache
    generic map (WordsPerLine => 4, NumberOfLines => 16, WriteThrough => true)
    port map (
      clk_i           => clk_i,
      mem_clk_i       => mem_clk,
      reset_n_i       => reset_n_i,
      slave_i         => wt_slave_in,
      slave_o         => wt_slave_out,
      snooping_addr_i => wt_inv_addr_i,
      snooping_en_i   => wt_inv_en_i,
      master_out_i    => wt_m_in,
      master_out_o    => wt_m_out);

  DUT_BYP : entity work.wb_cache
    generic map (
      WordsPerLine   => 4,
      NumberOfLines  => 16,
      WriteThrough   => true,
      EnableBypass   => true,
      BypassBaseAddr => 48)
    port map (
      clk_i           => clk_i,
      mem_clk_i       => mem_clk,
      reset_n_i       => reset_n_i,
      slave_i         => byp_slave_in,
      slave_o         => byp_slave_out,
      snooping_addr_i => to_unsigned(0, 32),
      snooping_en_i   => '0',
      master_out_i    => byp_m_in,
      master_out_o    => byp_m_out);

end architecture rtl;
