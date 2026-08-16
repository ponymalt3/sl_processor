-------------------------------------------------------------------------------
-- Title      : Testbench for design "wb_wm8731_slave"
-- Project    : 
-------------------------------------------------------------------------------
-- File       : wb_wm8731_slave_tb.vhd
-- Author     : malte  <malte@tp13>
-- Company    : 
-- Created    : 2019-01-20
-- Last update: 2019-02-05
-- Platform   : 
-- Standard   : VHDL'93/02
-------------------------------------------------------------------------------
-- Description: 
-------------------------------------------------------------------------------
-- Copyright (c) 2019 
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author  Description
-- 2019-01-20  1.0      malte	Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.textio.all;

use work.wishbone_p.all;
use work.wb_test_p.all;

entity wb_wm8731_slave_tb is
end entity wb_wm8731_slave_tb;

architecture wb_wm8731_tb of wb_wm8731_slave_tb is

  -- component ports
  signal clk          : std_ulogic := '1';
  signal reset_n      : std_ulogic;
  signal slave_in     : wb_slave_ifc_in_t;
  signal slave_out    : wb_slave_ifc_out_t;
  signal cfg_addr     : std_ulogic_vector(6 downto 0);
  signal cfg_data     : std_ulogic_vector(8 downto 0);
  signal cfg_idle     : std_ulogic;
  signal cfg_en       : std_ulogic;
  signal cfg_error    : std_ulogic;
  signal dsp_dacl     : std_ulogic_vector(23 downto 0);
  signal dsp_dacr     : std_ulogic_vector(23 downto 0);
  signal dsp_dac_sync : std_ulogic;
  signal dsp_adcl     : std_ulogic_vector(23 downto 0);
  signal dsp_adcr     : std_ulogic_vector(23 downto 0);
  signal dsp_adc_sync : std_ulogic;

  signal wm_mclk      : std_ulogic;
  signal wm_bclk      : std_ulogic;
  signal wm_adc_lrc   : std_ulogic;
  signal wm_dac_lrc   : std_ulogic;
  signal wm_adc       : std_ulogic;
  signal wm_dac       : std_ulogic;
  signal wm_sclk      : std_ulogic;
  signal wm_sdat      : std_logic;

  signal m_in : master_in_t;
  signal m_out : master_out_t;

  signal wm_in : std_ulogic_vector(47 downto 0);
  signal wm_in_complete : std_ulogic;

  signal wm_out : std_ulogic_vector(47 downto 0);
  signal wm_out_complete : std_ulogic;

begin  -- architecture wb_wm8731_tb

  wb_master_1: entity work.wb_master
    port map (
      clk_i        => clk,
      reset_n_i    => reset_n,
      addr_i       => m_out.addr,
      din_i        => m_out.dout,
      dout_o       => m_in.din,
      en_i         => m_out.en,
      burst_i      => m_out.burst,
      we_i         => m_out.we,
      dready_o     => m_in.dready,
      complete_o   => m_in.complete,
      err_o        => m_in.err,
      master_out_i => slave_out,
      master_out_o => slave_in);

  -- component instantiation
  DUT: entity work.wb_wm8731_slave
    port map (
      clk_i          => clk,
      reset_n_i      => reset_n,
      slave_i        => slave_in,
      slave_o        => slave_out,
      cfg_addr_o     => cfg_addr,
      cfg_data_o     => cfg_data,
      cfg_idle_i     => cfg_idle,
      cfg_en_o       => cfg_en,
      cfg_error_i    => cfg_error,
      dsp_dacl_o     => dsp_dacl,
      dsp_dacr_o     => dsp_dacr,
      dsp_dac_sync_i => dsp_dac_sync,
      dsp_adcl_i     => dsp_adcl,
      dsp_adcr_i     => dsp_adcr,
      dsp_adc_sync_i => dsp_adc_sync);

  wm8731_1: entity work.wm8731
    generic map (
      ClkPerMCLK => 4) -- 12.288Mhz => 12.5Mhz
    port map (
      clk_i          => clk,
      reset_n_i      => reset_n,
      wm_mclk_o      => wm_mclk,
      wm_bclk_o      => wm_bclk,
      wm_adc_lrc_o   => wm_adc_lrc,
      wm_dac_lrc_o   => wm_dac_lrc,
      wm_adc_i       => wm_adc,
      wm_dac_o       => wm_dac,
      wm_sclk        => wm_sclk,
      wm_sdat        => wm_sdat,
      cfg_addr_i     => cfg_addr,
      cfg_data_i     => cfg_data,
      cfg_idle_o     => cfg_idle,
      cfg_en_i       => cfg_en,
      cfg_error_o    => cfg_error,
      dsp_dacl_i     => dsp_dacl,
      dsp_dacr_i     => dsp_dacr,
      dsp_dac_sync_o => dsp_dac_sync,
      dsp_adcl_o     => dsp_adcl,
      dsp_adcr_o     => dsp_adcr,
      dsp_adc_sync_o => dsp_adc_sync,
      dbg_o          => open);

  process is
  begin  -- process
    wait until rising_edge(wm_bclk);
    wm_in_complete <= '0';
    
    if wm_adc_lrc = '1' then
      wait until falling_edge(wm_bclk);

      for i in 47 downto 0 loop
        wm_adc <= wm_in(i);
        wait until falling_edge(wm_bclk);
      end loop;  -- i
      wm_in_complete <= '1';
    end if;
  end process;

  process is
  begin  -- process
    wait until rising_edge(wm_bclk);
    wm_out_complete <= '0';
    
    if wm_dac_lrc = '1' then
      wait until falling_edge(wm_bclk);

      for i in 47 downto 0 loop
        wm_out(i) <= wm_dac;
        wait until falling_edge(wm_bclk);
      end loop;  -- i
      wm_out_complete <= '1';
    end if;
  end process;

  process (wm_sdat) is
  begin  -- process
    if wm_sdat = 'Z' then
      wm_sdat <= '0';
    elsif wm_sdat = 'X' or wm_sdat = 'U' then
      wm_sdat <= 'Z';
    end if;
  end process;

  -- clock generation
  clk <= not clk after 10 ns;

  process
    variable result : std_ulogic_vector(31 downto 0);
    variable measure_start : time;
  begin
    reset_n <= '0';
    wm_in <= (others => '0');

    wait for 32.6 ns;

    reset_n <= '1';

    wait until rising_edge(clk);

    -- wait until wm8731 self init is complete 
    master_access(0,X"00000000",result,'0',m_in,m_out);
    while result(0) /= '1' loop
      master_access(0,X"00000000",result,'0',m_in,m_out);
    end loop;
    

    -- sample rate set 32kHz
    master_access(0,X"60880018",result,'1',m_in,m_out);

    wait for 100 ns;

    -- wait until wm8731 config is complete 
    master_access(0,X"00000000",result,'0',m_in,m_out);
    while result(0) /= '1' loop
      master_access(0,X"00000000",result,'0',m_in,m_out);
    end loop;
    

    -- activate
    master_access(0,X"60890001",result,'1',m_in,m_out);

    -- wait until wm8731 config is complete 
    master_access(0,X"00000000",result,'0',m_in,m_out);
    while result(0) /= '1' loop
      master_access(0,X"00000000",result,'0',m_in,m_out);
    end loop;

    wait until rising_edge(wm_dac_lrc);
    measure_start := now;
    wait until rising_edge(wm_dac_lrc);
    assert (now-measure_start) > 30.5us and (now-measure_start) < 32us report "expect dac sample rate to be 32kHz" severity error;

    wait until rising_edge(wm_adc_lrc);
    measure_start := now;
    wait until rising_edge(wm_adc_lrc);
    assert (now-measure_start) > 30.5us and (now-measure_start) < 32us report "expect adc sample rate to be 32kHz" severity error;
    
    -- set adc data
    wait until rising_edge(wm_in_complete);
    wm_in <= X"ABEEF3BDEAD5";

    -- clear adc sync
    master_access(0,X"00000000",result,'0',m_in,m_out);

    -- wait for wm8731 adc sync
    master_access(0,X"00000000",result,'0',m_in,m_out);
    while result(2) /= '1' loop
      master_access(0,X"00000000",result,'0',m_in,m_out);
    end loop;

    master_access(1,X"00000000",result,'0',m_in,m_out);
    assert result = X"60ABEEF3" report "expect adc left channel to be 0x60ABEEF3" severity error;

    master_access(2,X"00000000",result,'0',m_in,m_out);
    assert result = X"60BDEAD5" report "expect adc right channel to be 0x60BDEAD5" severity error;

    wait until rising_edge(wm_in_complete);
    wm_in <= X"000000000000";
    

    -- set dac data
    wait until rising_edge(wm_out_complete);
    master_access(1,X"60CFEEB7",result,'1',m_in,m_out);
    master_access(2,X"60DDAED9",result,'1',m_in,m_out);
    -- clear dac sync
    master_access(0,X"00000000",result,'0',m_in,m_out);

    -- wait for wm8731 dac sync
    master_access(0,X"00000000",result,'0',m_in,m_out);
    while result(3) /= '1' loop
      master_access(0,X"00000000",result,'0',m_in,m_out);
    end loop;

    wait until wm_out_complete = '1';

    assert wm_out(47 downto 24) = X"CFEEB7" report "expect dac left channel to be 0xCFEEB7" severity error;
    assert wm_out(23 downto 0) = X"DDAED9" report "expect dac right channel to be 0xDDAED9" severity error;

    
    
    write(output,"all tests complete" & LF);

    reset_n <= '0';
    
    wait;
    
  end process;

  

end architecture wb_wm8731_tb;
