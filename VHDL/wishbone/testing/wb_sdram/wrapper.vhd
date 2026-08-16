-- Flat-port wrapper around wb_sdram for cocotb.
--
-- Two DUTs, each behind a real wb_master (the same master wb_cache uses, so
-- the bursts on the Wishbone side look exactly like cache line traffic):
--   a_*  normal refresh interval -- functional + throughput checks
--   b_*  1us refresh interval    -- refresh keeps interrupting bursts
--
-- sdram_dq_io is not exported: GHDL's VPI does not resolve a value cocotb
-- writes to an inout port against that port's own RTL driver (see
-- sdram_model.py's docstring), so the model's tri-state mux is done here in
-- VHDL as a second real driver on an internal dq_bus net, and cocotb only
-- touches the plain single-driver *_sdram_model_* ports.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wishbone_p.all;

entity wb_sdram_wrapper is
  port (
    clk_i     : in std_ulogic;
    reset_n_i : in std_ulogic;

    a_addr_i     : in  unsigned(31 downto 0);
    a_din_i      : in  std_ulogic_vector(31 downto 0);
    a_dout_o     : out std_ulogic_vector(31 downto 0);
    a_en_i       : in  std_ulogic;
    a_burst_i    : in  unsigned(5 downto 0);
    a_we_i       : in  std_ulogic;
    a_dready_o   : out std_ulogic;
    a_complete_o : out std_ulogic;

    a_sdram_cs_n_o  : out std_ulogic;
    a_sdram_ras_n_o : out std_ulogic;
    a_sdram_cas_n_o : out std_ulogic;
    a_sdram_we_n_o  : out std_ulogic;
    a_sdram_ba_o    : out std_ulogic_vector(1 downto 0);
    a_sdram_addr_o  : out std_ulogic_vector(12 downto 0);
    a_sdram_dqm_o   : out std_ulogic_vector(1 downto 0);

    a_sdram_model_din_o  : out std_ulogic_vector(15 downto 0);
    a_sdram_model_dout_i : in  std_ulogic_vector(15 downto 0);
    a_sdram_model_oe_i   : in  std_ulogic;

    b_addr_i     : in  unsigned(31 downto 0);
    b_din_i      : in  std_ulogic_vector(31 downto 0);
    b_dout_o     : out std_ulogic_vector(31 downto 0);
    b_en_i       : in  std_ulogic;
    b_burst_i    : in  unsigned(5 downto 0);
    b_we_i       : in  std_ulogic;
    b_dready_o   : out std_ulogic;
    b_complete_o : out std_ulogic;

    b_sdram_cs_n_o  : out std_ulogic;
    b_sdram_ras_n_o : out std_ulogic;
    b_sdram_cas_n_o : out std_ulogic;
    b_sdram_we_n_o  : out std_ulogic;
    b_sdram_ba_o    : out std_ulogic_vector(1 downto 0);
    b_sdram_addr_o  : out std_ulogic_vector(12 downto 0);
    b_sdram_dqm_o   : out std_ulogic_vector(1 downto 0);

    b_sdram_model_din_o  : out std_ulogic_vector(15 downto 0);
    b_sdram_model_dout_i : in  std_ulogic_vector(15 downto 0);
    b_sdram_model_oe_i   : in  std_ulogic);
end entity wb_sdram_wrapper;

architecture rtl of wb_sdram_wrapper is

  signal a_dq_bus : std_logic_vector(15 downto 0);
  signal b_dq_bus : std_logic_vector(15 downto 0);

  signal a_m_in  : wb_master_ifc_in_t;
  signal a_m_out : wb_master_ifc_out_t;
  signal b_m_in  : wb_master_ifc_in_t;
  signal b_m_out : wb_master_ifc_out_t;

begin

  a_dq_bus <= To_StdLogicVector(a_sdram_model_dout_i) when a_sdram_model_oe_i = '1'
              else (others => 'Z');
  a_sdram_model_din_o <= To_StdULogicVector(a_dq_bus);

  b_dq_bus <= To_StdLogicVector(b_sdram_model_dout_i) when b_sdram_model_oe_i = '1'
              else (others => 'Z');
  b_sdram_model_din_o <= To_StdULogicVector(b_dq_bus);

  wb_master_a: entity work.wb_master
    port map (
      clk_i        => clk_i,
      reset_n_i    => reset_n_i,
      addr_i       => a_addr_i,
      din_i        => a_din_i,
      dout_o       => a_dout_o,
      en_i         => a_en_i,
      burst_i      => a_burst_i,
      we_i         => a_we_i,
      dready_o     => a_dready_o,
      complete_o   => a_complete_o,
      err_o        => open,
      master_out_i => a_m_in,
      master_out_o => a_m_out);

  -- InitDelayUs shortened from the real 100us: the init sequence itself is
  -- what is under test, not how long the power-up wait is.
  wb_sdram_a: entity work.wb_sdram
    generic map (
      ClockFreqHz => 50_000_000,
      InitDelayUs => 1.0)
    port map (
      clk_i         => clk_i,
      reset_n_i     => reset_n_i,
      slave_i       => a_m_out,
      slave_o       => a_m_in,
      sdram_clk_o   => open,
      sdram_cke_o   => open,
      sdram_cs_n_o  => a_sdram_cs_n_o,
      sdram_ras_n_o => a_sdram_ras_n_o,
      sdram_cas_n_o => a_sdram_cas_n_o,
      sdram_we_n_o  => a_sdram_we_n_o,
      sdram_ba_o    => a_sdram_ba_o,
      sdram_addr_o  => a_sdram_addr_o,
      sdram_dqm_o   => a_sdram_dqm_o,
      sdram_dq_io   => a_dq_bus);

  wb_master_b: entity work.wb_master
    port map (
      clk_i        => clk_i,
      reset_n_i    => reset_n_i,
      addr_i       => b_addr_i,
      din_i        => b_din_i,
      dout_o       => b_dout_o,
      en_i         => b_en_i,
      burst_i      => b_burst_i,
      we_i         => b_we_i,
      dready_o     => b_dready_o,
      complete_o   => b_complete_o,
      err_o        => open,
      master_out_i => b_m_in,
      master_out_o => b_m_out);

  wb_sdram_b: entity work.wb_sdram
    generic map (
      ClockFreqHz       => 50_000_000,
      InitDelayUs       => 1.0,
      RefreshIntervalUs => 1.0)
    port map (
      clk_i         => clk_i,
      reset_n_i     => reset_n_i,
      slave_i       => b_m_out,
      slave_o       => b_m_in,
      sdram_clk_o   => open,
      sdram_cke_o   => open,
      sdram_cs_n_o  => b_sdram_cs_n_o,
      sdram_ras_n_o => b_sdram_ras_n_o,
      sdram_cas_n_o => b_sdram_cas_n_o,
      sdram_we_n_o  => b_sdram_we_n_o,
      sdram_ba_o    => b_sdram_ba_o,
      sdram_addr_o  => b_sdram_addr_o,
      sdram_dqm_o   => b_sdram_dqm_o,
      sdram_dq_io   => b_dq_bus);

end architecture rtl;
