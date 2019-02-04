onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate /wb_wm8731_slave_tb/clk
add wave -noupdate /wb_wm8731_slave_tb/reset_n
add wave -noupdate -expand /wb_wm8731_slave_tb/slave_in
add wave -noupdate /wb_wm8731_slave_tb/slave_out
add wave -noupdate /wb_wm8731_slave_tb/cfg_addr
add wave -noupdate /wb_wm8731_slave_tb/cfg_data
add wave -noupdate /wb_wm8731_slave_tb/cfg_idle
add wave -noupdate /wb_wm8731_slave_tb/cfg_en
add wave -noupdate /wb_wm8731_slave_tb/cfg_error
add wave -noupdate /wb_wm8731_slave_tb/dsp_dacl
add wave -noupdate /wb_wm8731_slave_tb/dsp_dacr
add wave -noupdate /wb_wm8731_slave_tb/dsp_dac_sync
add wave -noupdate /wb_wm8731_slave_tb/dsp_adcl
add wave -noupdate /wb_wm8731_slave_tb/dsp_adcr
add wave -noupdate /wb_wm8731_slave_tb/dsp_adc_sync
add wave -noupdate /wb_wm8731_slave_tb/wm_mclk
add wave -noupdate /wb_wm8731_slave_tb/wm_bclk
add wave -noupdate /wb_wm8731_slave_tb/wm_adc_lrc
add wave -noupdate /wb_wm8731_slave_tb/wm_dac_lrc
add wave -noupdate /wb_wm8731_slave_tb/wm_adc
add wave -noupdate /wb_wm8731_slave_tb/wm_dac
add wave -noupdate /wb_wm8731_slave_tb/wm_sclk
add wave -noupdate /wb_wm8731_slave_tb/wm_sdat
add wave -noupdate -childformat {{/wb_wm8731_slave_tb/m_in.din -radix hexadecimal}} -expand -subitemconfig {/wb_wm8731_slave_tb/m_in.din {-height 16 -radix hexadecimal}} /wb_wm8731_slave_tb/m_in
add wave -noupdate -childformat {{/wb_wm8731_slave_tb/m_out.addr -radix unsigned} {/wb_wm8731_slave_tb/m_out.dout -radix hexadecimal} {/wb_wm8731_slave_tb/m_out.burst -radix unsigned}} -expand -subitemconfig {/wb_wm8731_slave_tb/m_out.addr {-height 16 -radix unsigned} /wb_wm8731_slave_tb/m_out.dout {-height 16 -radix hexadecimal} /wb_wm8731_slave_tb/m_out.burst {-height 16 -radix unsigned}} /wb_wm8731_slave_tb/m_out
add wave -noupdate /wb_wm8731_slave_tb/wm_in
add wave -noupdate /wb_wm8731_slave_tb/wm_in_complete
add wave -noupdate /wb_wm8731_slave_tb/wm_out
add wave -noupdate /wb_wm8731_slave_tb/wm_out_complete
add wave -noupdate -divider DUT
add wave -noupdate /wb_wm8731_slave_tb/DUT/clk_i
add wave -noupdate /wb_wm8731_slave_tb/DUT/reset_n_i
add wave -noupdate -childformat {{/wb_wm8731_slave_tb/DUT/slave_i.adr -radix unsigned} {/wb_wm8731_slave_tb/DUT/slave_i.dat -radix hexadecimal}} -expand -subitemconfig {/wb_wm8731_slave_tb/DUT/slave_i.adr {-height 16 -radix unsigned} /wb_wm8731_slave_tb/DUT/slave_i.dat {-height 16 -radix hexadecimal}} /wb_wm8731_slave_tb/DUT/slave_i
add wave -noupdate -expand /wb_wm8731_slave_tb/DUT/slave_o
add wave -noupdate /wb_wm8731_slave_tb/DUT/cfg_addr_o
add wave -noupdate /wb_wm8731_slave_tb/DUT/cfg_data_o
add wave -noupdate /wb_wm8731_slave_tb/DUT/cfg_idle_i
add wave -noupdate /wb_wm8731_slave_tb/DUT/cfg_en_o
add wave -noupdate /wb_wm8731_slave_tb/DUT/cfg_error_i
add wave -noupdate /wb_wm8731_slave_tb/DUT/dsp_dacl_o
add wave -noupdate /wb_wm8731_slave_tb/DUT/dsp_dacr_o
add wave -noupdate /wb_wm8731_slave_tb/DUT/dsp_dac_sync_i
add wave -noupdate /wb_wm8731_slave_tb/DUT/dsp_adcl_i
add wave -noupdate /wb_wm8731_slave_tb/DUT/dsp_adcr_i
add wave -noupdate /wb_wm8731_slave_tb/DUT/dsp_adc_sync_i
add wave -noupdate /wb_wm8731_slave_tb/DUT/sfr_cmd
add wave -noupdate /wb_wm8731_slave_tb/DUT/datl_in_reg
add wave -noupdate /wb_wm8731_slave_tb/DUT/datr_in_reg
add wave -noupdate /wb_wm8731_slave_tb/DUT/datl_out_reg
add wave -noupdate /wb_wm8731_slave_tb/DUT/datr_out_reg
add wave -noupdate /wb_wm8731_slave_tb/DUT/ch_full_l
add wave -noupdate /wb_wm8731_slave_tb/DUT/ch_full_r
add wave -noupdate /wb_wm8731_slave_tb/DUT/adc_sync
add wave -noupdate /wb_wm8731_slave_tb/DUT/dac_sync
add wave -noupdate /wb_wm8731_slave_tb/DUT/wm8731_conf_en
add wave -noupdate -divider wm8731
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/clk_i
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/reset_n_i
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/wm_mclk_o
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/wm_bclk_o
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/wm_adc_lrc_o
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/wm_dac_lrc_o
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/wm_adc_i
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/wm_dac_o
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/wm_sclk
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/wm_sdat
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/cfg_addr_i
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/cfg_data_i
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/cfg_idle_o
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/cfg_en_i
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/cfg_error_o
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/dsp_dacl_i
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/dsp_dacr_i
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/dsp_dac_sync_o
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/dsp_adcl_o
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/dsp_adcr_o
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/dsp_adc_sync_o
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/dbg_o
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/state
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/mclk
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/adc_reg
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/dac_reg
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/cfg_reg
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/samp_rate
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/mclk_count
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/sync_count
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/sync_count_max
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/i2c_active
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/i2c_busy
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/i2c_start
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/i2c_din
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/i2c_we
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/i2c_ack
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/i2c_error
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/cfg_valid
add wave -noupdate -radix binary /wb_wm8731_slave_tb/wm8731_1/cfg_din
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/sr_count
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/i2c_reconf_en
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/cfg_sr_valid
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/frame_en
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/frame_sync
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/frame_full
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/lrc
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/clk_edge
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/clk_edge_1d
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/cfg_data
add wave -noupdate /wb_wm8731_slave_tb/wm8731_1/cfg_addr
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {555120000 ps} 1} {{Cursor 2} {1969880000 ps} 0}
quietly wave cursor active 2
configure wave -namecolwidth 150
configure wave -valuecolwidth 100
configure wave -justifyvalue left
configure wave -signalnamewidth 1
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 0
configure wave -gridperiod 1
configure wave -griddelta 40
configure wave -timeline 0
configure wave -timelineunits us
update
WaveRestoreZoom {0 ps} {4211081216 ps}
