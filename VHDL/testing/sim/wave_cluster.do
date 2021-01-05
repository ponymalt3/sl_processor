onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate /sl_cluster_tb/clk
add wave -noupdate /sl_cluster_tb/reset_n
add wave -noupdate /sl_cluster_tb/core_en
add wave -noupdate /sl_cluster_tb/core_reset_n
add wave -noupdate /sl_cluster_tb/wbs_in
add wave -noupdate -childformat {{/sl_cluster_tb/wbs_out.dat -radix hexadecimal}} -expand -subitemconfig {/sl_cluster_tb/wbs_out.dat {-height 16 -radix hexadecimal}} /sl_cluster_tb/wbs_out
add wave -noupdate /sl_cluster_tb/ext_master_in
add wave -noupdate /sl_cluster_tb/ext_master_out
add wave -noupdate /sl_cluster_tb/code_master_in
add wave -noupdate /sl_cluster_tb/code_master_out
add wave -noupdate /sl_cluster_tb/master_in
add wave -noupdate /sl_cluster_tb/master_out
add wave -noupdate /sl_cluster_tb/slave_in
add wave -noupdate /sl_cluster_tb/slave_out
add wave -noupdate /sl_cluster_tb/mem_clk
add wave -noupdate /sl_cluster_tb/code
add wave -noupdate /sl_cluster_tb/ExtMemAddr
add wave -noupdate -childformat {{/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(31) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(30) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(29) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(28) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(27) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(26) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(25) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(24) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(23) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(22) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(21) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(20) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(19) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(18) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(17) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(16) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(15) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(14) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(13) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(12) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(11) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(10) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(9) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(8) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(7) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(6) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(5) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(4) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(3) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(1) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(0) -radix hexadecimal}} -subitemconfig {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(31) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(30) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(29) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(28) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(27) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(26) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(25) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(24) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(23) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(22) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(21) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(20) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(19) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(18) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(17) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(16) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(15) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(14) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(13) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(12) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(11) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(10) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(9) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(8) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(7) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(6) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(5) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(4) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(3) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(1) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(0) {-height 16 -radix hexadecimal}} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem
add wave -noupdate -divider wbmem
add wave -noupdate /sl_cluster_tb/wb_mem_1/clk_i
add wave -noupdate /sl_cluster_tb/wb_mem_1/mem_clk_i
add wave -noupdate /sl_cluster_tb/wb_mem_1/reset_n_i
add wave -noupdate -childformat {{/sl_cluster_tb/wb_mem_1/slave0_i.adr -radix unsigned}} -expand -subitemconfig {/sl_cluster_tb/wb_mem_1/slave0_i.adr {-height 16 -radix unsigned}} /sl_cluster_tb/wb_mem_1/slave0_i
add wave -noupdate -childformat {{/sl_cluster_tb/wb_mem_1/slave0_o.dat -radix hexadecimal}} -expand -subitemconfig {/sl_cluster_tb/wb_mem_1/slave0_o.dat {-height 16 -radix hexadecimal}} /sl_cluster_tb/wb_mem_1/slave0_o
add wave -noupdate -childformat {{/sl_cluster_tb/wb_mem_1/slave1_i.adr -radix unsigned}} -expand -subitemconfig {/sl_cluster_tb/wb_mem_1/slave1_i.adr {-height 16 -radix unsigned}} /sl_cluster_tb/wb_mem_1/slave1_i
add wave -noupdate -childformat {{/sl_cluster_tb/wb_mem_1/slave1_o.dat -radix hexadecimal}} -expand -subitemconfig {/sl_cluster_tb/wb_mem_1/slave1_o.dat {-height 16 -radix hexadecimal}} /sl_cluster_tb/wb_mem_1/slave1_o
add wave -noupdate /sl_cluster_tb/wb_mem_1/slave0_in
add wave -noupdate /sl_cluster_tb/wb_mem_1/slave1_in
add wave -noupdate /sl_cluster_tb/wb_mem_1/mem_dout0
add wave -noupdate /sl_cluster_tb/wb_mem_1/mem_dout1
add wave -noupdate -childformat {{/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2054) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2053) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2052) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2051) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2050) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2049) -radix hexadecimal} {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2048) -radix hexadecimal}} -subitemconfig {/sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2054) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2053) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2052) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2051) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2050) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2049) {-height 16 -radix hexadecimal} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem(2048) {-height 16 -radix hexadecimal}} /sl_cluster_tb/wb_mem_1/sl_dpram_1/mem
add wave -noupdate -divider {mem system}
add wave -noupdate /sl_cluster_tb/wb_ixs_1/clk_i
add wave -noupdate /sl_cluster_tb/wb_ixs_1/reset_n_i
add wave -noupdate -expand -subitemconfig {/sl_cluster_tb/wb_ixs_1/master_in_i(1) {-height 16 -childformat {{/sl_cluster_tb/wb_ixs_1/master_in_i(1).adr -radix unsigned}} -expand} /sl_cluster_tb/wb_ixs_1/master_in_i(1).adr {-height 16 -radix unsigned} /sl_cluster_tb/wb_ixs_1/master_in_i(0) {-height 16 -childformat {{/sl_cluster_tb/wb_ixs_1/master_in_i(0).adr -radix unsigned} {/sl_cluster_tb/wb_ixs_1/master_in_i(0).dat -radix hexadecimal}} -expand} /sl_cluster_tb/wb_ixs_1/master_in_i(0).adr {-height 16 -radix unsigned} /sl_cluster_tb/wb_ixs_1/master_in_i(0).dat {-height 16 -radix hexadecimal}} /sl_cluster_tb/wb_ixs_1/master_in_i
add wave -noupdate -expand -subitemconfig {/sl_cluster_tb/wb_ixs_1/master_in_o(1) {-height 16 -childformat {{/sl_cluster_tb/wb_ixs_1/master_in_o(1).dat -radix hexadecimal}} -expand} /sl_cluster_tb/wb_ixs_1/master_in_o(1).dat {-height 16 -radix hexadecimal}} /sl_cluster_tb/wb_ixs_1/master_in_o
add wave -noupdate -expand -subitemconfig {/sl_cluster_tb/wb_ixs_1/slave_out_i(0) {-height 16 -childformat {{/sl_cluster_tb/wb_ixs_1/slave_out_i(0).dat -radix hexadecimal}} -expand} /sl_cluster_tb/wb_ixs_1/slave_out_i(0).dat {-height 16 -radix hexadecimal}} /sl_cluster_tb/wb_ixs_1/slave_out_i
add wave -noupdate -expand -subitemconfig {/sl_cluster_tb/wb_ixs_1/slave_out_o(0) {-height 16 -childformat {{/sl_cluster_tb/wb_ixs_1/slave_out_o(0).adr -radix unsigned} {/sl_cluster_tb/wb_ixs_1/slave_out_o(0).dat -radix hexadecimal}} -expand} /sl_cluster_tb/wb_ixs_1/slave_out_o(0).adr {-height 16 -radix unsigned} /sl_cluster_tb/wb_ixs_1/slave_out_o(0).dat {-height 16 -radix hexadecimal}} /sl_cluster_tb/wb_ixs_1/slave_out_o
add wave -noupdate /sl_cluster_tb/wb_ixs_1/decode_ifc_in
add wave -noupdate /sl_cluster_tb/wb_ixs_1/decode_ifc_out
add wave -noupdate -divider arb
add wave -noupdate /sl_cluster_tb/wb_ixs_1/arbiter(0)/m_in
add wave -noupdate /sl_cluster_tb/wb_ixs_1/arbiter(0)/m_out
add wave -noupdate /sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/clk_i
add wave -noupdate /sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/reset_n_i
add wave -noupdate -expand -subitemconfig {/sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/master_in_i(1) -expand /sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/master_in_i(0) -expand} /sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/master_in_i
add wave -noupdate /sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/master_in_o
add wave -noupdate /sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/master_out_i
add wave -noupdate /sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/master_out_o
add wave -noupdate /sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/mask
add wave -noupdate /sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/master_sel
add wave -noupdate /sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/master_sel_reg
add wave -noupdate /sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/master_sel_valid
add wave -noupdate /sl_cluster_tb/wb_ixs_1/arbiter(0)/wb_ixs_arbiter/master_sel_valid_reg
add wave -noupdate -divider {cache 0}
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/clk_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/mem_clk_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/reset_n_i
add wave -noupdate -radix unsigned /sl_cluster_tb/DUT/proc(0)/wb_cache_1/addr_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/din_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/dout_o
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/en_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/we_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/complete_o
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/err_o
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/snooping_addr_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/snooping_en_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/master_out_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/master_out_o
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/state
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/count
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/tag_init_complete
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/mem_addr
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/mem_din
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/mem_we
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/mem1_addr
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/mem1_din
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/mem1_we
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/mem1_dout
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/mem_write_en
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/mem1_write_en
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/mem1_addr_ov_bit
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/cache_hit
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/wb_fetch
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/wb_writeback
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/tag_index
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/tag_in
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/tag_we
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/tag_out
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/tag1_re_and_invalidate
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/tag1_we
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/tag1_in
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/tag1_out
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/tag1_index
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/inv_addr
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/wb_en
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/wb_we
add wave -noupdate -radix unsigned /sl_cluster_tb/DUT/proc(0)/wb_cache_1/wb_addr
add wave -noupdate -radix hexadecimal /sl_cluster_tb/DUT/proc(0)/wb_cache_1/wb_din
add wave -noupdate -radix hexadecimal /sl_cluster_tb/DUT/proc(0)/wb_cache_1/wb_dout
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/wb_complete
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/wb_dready
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/wb_err
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/wb_burst
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/fetch_ignore_counter
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/fetch_ignore_addr
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/line_active
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/active_line_addr
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/en_and_line_inactive
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/we_1d
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/pending_write
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/pending_write_1d
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/addr_ov_bit
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/xxx
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/en
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/wb_cache_1/snooping_en
add wave -noupdate -divider {core 0}
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/clk_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/mem_clk_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/reset_n_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/core_en_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/core_reset_n_i
add wave -noupdate -radix unsigned /sl_cluster_tb/DUT/proc(0)/sl_processor_1/code_addr_o
add wave -noupdate -radix hexadecimal /sl_cluster_tb/DUT/proc(0)/sl_processor_1/code_data_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/ext_master_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/ext_master_o
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/debug_slave_i
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/debug_slave_o
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/executed_addr_o
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/core_clk
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/core_en_1d
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/alu_en
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/alu_cmd
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/alu_op_a
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/alu_op_b
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/alu_data
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/cp_addr
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/rp0_addr
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/rp0_dout
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/rp0_en
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/rp1_addr
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/rp1_dout
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/wp_addr
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/wp_din
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/wp_we
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/qfp_cmd
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/qfp_ready
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/qfp_result
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/qfp_cmp_gt
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/qfp_cmp_z
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/qfp_complete
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/qfp_int_result
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/qfp_idle
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/alu_en2
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/multi_cycle_op
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/rp_stall
add wave -noupdate -radix unsigned /sl_cluster_tb/DUT/proc(0)/sl_processor_1/ext_mem_addr
add wave -noupdate -radix hexadecimal /sl_cluster_tb/DUT/proc(0)/sl_processor_1/ext_mem_dout
add wave -noupdate -radix hexadecimal /sl_cluster_tb/DUT/proc(0)/sl_processor_1/ext_mem_din
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/ext_mem_rw
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/ext_mem_en
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/ext_mem_stall
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/mem_we
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/mem_addr
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/mem1_we
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/mem1_addr
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/mem_complete
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/mem_slave_ack
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/ext_mem_disable
add wave -noupdate /sl_cluster_tb/DUT/proc(0)/sl_processor_1/ext_mem_en_1d
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 2} {8051847 ps} 0} {{Cursor 3} {8397425 ps} 1}
quietly wave cursor active 1
configure wave -namecolwidth 195
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
WaveRestoreZoom {8067580 ps} {8564864 ps}
