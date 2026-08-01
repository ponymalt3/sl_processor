-- Flat-port wrapper around wb_cache for cocotb.
-- wb_/wt_/byp_ wire straight into wb_cache's flat interface. ix_ goes
-- through a real wb_master -> wb_ixs -> wb_cache_adapter, the one variant
-- that exercises a real wb_ixs.
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

    -- Write-through + bypass cache (byp_*), mirrors sl_cluster.vhd's data
    -- cache. BypassBaseAddr=48 sits inside the cache's own indexable range.
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
    byp_m_cyc_o : out std_ulogic;

    -- Write-back cache reached through a real single-master wb_ixs, same
    -- topology as sl_system.vhd's debug_ctrl -> wb_ixs_1 -> sdram_cache_1.
    ix_addr_i     : in  unsigned(31 downto 0);
    ix_din_i      : in  std_ulogic_vector(31 downto 0);
    ix_dout_o     : out std_ulogic_vector(31 downto 0);
    ix_en_i       : in  std_ulogic;
    ix_burst_i    : in  unsigned(5 downto 0);
    ix_we_i       : in  std_ulogic;
    ix_dready_o   : out std_ulogic;
    ix_complete_o : out std_ulogic;

    ix_m_dat_i   : in  std_ulogic_vector(31 downto 0);
    ix_m_ack_i   : in  std_ulogic;
    ix_m_err_i   : in  std_ulogic;
    ix_m_stall_i : in  std_ulogic;

    ix_m_adr_o : out unsigned(31 downto 0);
    ix_m_dat_o : out std_ulogic_vector(31 downto 0);
    ix_m_we_o  : out std_ulogic;
    ix_m_stb_o : out std_ulogic;
    ix_m_cyc_o : out std_ulogic;

    -- Write-back cache with NarrowTag and 8 words/line -- the tag geometry
    -- (TagWidth/TagAddrHi/TagAddrLo/TagLow) that sl_cluster.vhd's L1s use.
    nt_addr_i     : in  unsigned(31 downto 0);
    nt_din_i      : in  std_ulogic_vector(31 downto 0);
    nt_dout_o     : out std_ulogic_vector(31 downto 0);
    nt_en_i       : in  std_ulogic;
    nt_we_i       : in  std_ulogic;
    nt_complete_o : out std_ulogic;

    nt_m_dat_i   : in  std_ulogic_vector(31 downto 0);
    nt_m_ack_i   : in  std_ulogic;
    nt_m_err_i   : in  std_ulogic;
    nt_m_stall_i : in  std_ulogic;

    nt_m_adr_o : out unsigned(31 downto 0);
    nt_m_dat_o : out std_ulogic_vector(31 downto 0);
    nt_m_we_o  : out std_ulogic;
    nt_m_stb_o : out std_ulogic;
    nt_m_cyc_o : out std_ulogic
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
  signal nt_m_in   : wb_master_ifc_in_t;
  signal nt_m_out  : wb_master_ifc_out_t;

  constant IxMasterConfig : wb_master_config_array_t := (0 => wb_master("cache"));
  constant IxSlaveMap     : wb_slave_config_array_t  := (0 => wb_slave("cache", 0, 256));

  signal ix_req_m_in  : wb_master_ifc_in_t;
  signal ix_req_m_out : wb_master_ifc_out_t;

  signal ix_master_in  : wb_slave_ifc_in_array_t(0 downto 0);
  signal ix_master_out : wb_slave_ifc_out_array_t(0 downto 0);
  signal ix_cache_req  : wb_slave_ifc_in_array_t(0 downto 0);
  signal ix_cache_resp : wb_slave_ifc_out_array_t(0 downto 0);

  signal ix_m_in  : wb_master_ifc_in_t;
  signal ix_m_out : wb_master_ifc_out_t;

  signal ix_cache_addr     : unsigned(31 downto 0);
  signal ix_cache_wdata    : std_ulogic_vector(31 downto 0);
  signal ix_cache_rdata    : std_ulogic_vector(31 downto 0);
  signal ix_cache_en       : std_ulogic;
  signal ix_cache_we       : std_ulogic;
  signal ix_cache_complete : std_ulogic;
  signal ix_cache_err      : std_ulogic;

begin

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

  nt_m_in.dat   <= nt_m_dat_i;
  nt_m_in.ack   <= nt_m_ack_i;
  nt_m_in.err   <= nt_m_err_i;
  nt_m_in.stall <= nt_m_stall_i;
  nt_m_adr_o    <= nt_m_out.adr;
  nt_m_dat_o    <= nt_m_out.dat;
  nt_m_we_o     <= nt_m_out.we;
  nt_m_stb_o    <= nt_m_out.stb;
  nt_m_cyc_o    <= nt_m_out.cyc;

  ix_m_in.dat   <= ix_m_dat_i;
  ix_m_in.ack   <= ix_m_ack_i;
  ix_m_in.err   <= ix_m_err_i;
  ix_m_in.stall <= ix_m_stall_i;
  ix_m_adr_o    <= ix_m_out.adr;
  ix_m_dat_o    <= ix_m_out.dat;
  ix_m_we_o     <= ix_m_out.we;
  ix_m_stb_o    <= ix_m_out.stb;
  ix_m_cyc_o    <= ix_m_out.cyc;

  wb_master_ix: entity work.wb_master
    port map (
      clk_i        => clk_i,
      reset_n_i    => reset_n_i,
      addr_i       => ix_addr_i,
      din_i        => ix_din_i,
      dout_o       => ix_dout_o,
      en_i         => ix_en_i,
      burst_i      => ix_burst_i,
      we_i         => ix_we_i,
      dready_o     => ix_dready_o,
      complete_o   => ix_complete_o,
      err_o        => open,
      master_out_i => ix_req_m_in,
      master_out_o => ix_req_m_out);

  ix_master_in(0) <= ix_req_m_out;
  ix_req_m_in     <= ix_master_out(0);

  wb_ixs_1: entity work.wb_ixs
    generic map (
      MasterConfig => IxMasterConfig,
      SlaveMap     => IxSlaveMap)
    port map (
      clk_i       => clk_i,
      reset_n_i   => reset_n_i,
      master_in_i => ix_master_in,
      master_in_o => ix_master_out,
      slave_out_i => ix_cache_resp,
      slave_out_o => ix_cache_req);

  wb_cache_adapter_ix: entity work.wb_cache_adapter
    port map (
      clk_i      => clk_i,
      reset_n_i  => reset_n_i,
      addr_o     => ix_cache_addr,
      din_i      => ix_cache_rdata,
      dout_o     => ix_cache_wdata,
      en_o       => ix_cache_en,
      we_o       => ix_cache_we,
      complete_i => ix_cache_complete,
      err_i      => ix_cache_err,
      slave_i    => ix_cache_req(0),
      slave_o    => ix_cache_resp(0));

  DUT_IX : entity work.wb_cache
    generic map (WordsPerLine => 4, NumberOfLines => 16, WriteThrough => false)
    port map (
      clk_i           => clk_i,
      mem_clk_i       => mem_clk,
      reset_n_i       => reset_n_i,
      addr_i          => ix_cache_addr,
      din_i           => ix_cache_wdata,
      dout_o          => ix_cache_rdata,
      en_i            => ix_cache_en,
      we_i            => ix_cache_we,
      complete_o      => ix_cache_complete,
      err_o           => ix_cache_err,
      snooping_addr_i => to_unsigned(0, 32),
      snooping_en_i   => '0',
      master_out_i    => ix_m_in,
      master_out_o    => ix_m_out);

  DUT_NT : entity work.wb_cache
    generic map (WordsPerLine => 8, NumberOfLines => 16,
                 WriteThrough => false, NarrowTag => true)
    port map (
      clk_i           => clk_i,
      mem_clk_i       => mem_clk,
      reset_n_i       => reset_n_i,
      addr_i          => nt_addr_i,
      din_i           => nt_din_i,
      dout_o          => nt_dout_o,
      en_i            => nt_en_i,
      we_i            => nt_we_i,
      complete_o      => nt_complete_o,
      err_o           => open,
      snooping_addr_i => to_unsigned(0, 32),
      snooping_en_i   => '0',
      master_out_i    => nt_m_in,
      master_out_o    => nt_m_out);

  DUT_WB : entity work.wb_cache
    generic map (WordsPerLine => 4, NumberOfLines => 16, WriteThrough => false)
    port map (
      clk_i           => clk_i,
      mem_clk_i       => mem_clk,
      reset_n_i       => reset_n_i,
      addr_i          => wb_addr_i,
      din_i           => wb_din_i,
      dout_o          => wb_dout_o,
      en_i            => wb_en_i,
      we_i            => wb_we_i,
      complete_o      => wb_complete_o,
      err_o           => open,
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
      addr_i          => wt_addr_i,
      din_i           => wt_din_i,
      dout_o          => wt_dout_o,
      en_i            => wt_en_i,
      we_i            => wt_we_i,
      complete_o      => wt_complete_o,
      err_o           => open,
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
      addr_i          => byp_addr_i,
      din_i           => byp_din_i,
      dout_o          => byp_dout_o,
      en_i            => byp_en_i,
      we_i            => byp_we_i,
      complete_o      => byp_complete_o,
      err_o           => open,
      snooping_addr_i => to_unsigned(0, 32),
      snooping_en_i   => '0',
      master_out_i    => byp_m_in,
      master_out_o    => byp_m_out);

end architecture rtl;
