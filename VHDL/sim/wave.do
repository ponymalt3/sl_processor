onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate /sl_processor_tb/DUT/sl_core_1/clk_i
add wave -noupdate /sl_processor_tb/DUT/clk_i
add wave -noupdate /sl_processor_tb/DUT/clk2
add wave -noupdate /sl_processor_tb/DUT/clk2
add wave -noupdate /sl_processor_tb/DUT/sl_core_1/reset_n_i
add wave -noupdate /sl_processor_tb/DUT/sl_core_1/alu_en_o
add wave -noupdate /sl_processor_tb/DUT/sl_core_1/alu_cmd_o
add wave -noupdate -radix hexadecimal /sl_processor_tb/DUT/sl_core_1/alu_op_a_o
add wave -noupdate -radix hexadecimal /sl_processor_tb/DUT/sl_core_1/alu_op_b_o
add wave -noupdate -childformat {{/sl_processor_tb/DUT/sl_core_1/alu_i.result -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/alu_i.int_result -radix hexadecimal}} -expand -subitemconfig {/sl_processor_tb/DUT/sl_core_1/alu_i.result {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/alu_i.int_result {-height 16 -radix hexadecimal}} /sl_processor_tb/DUT/sl_core_1/alu_i
add wave -noupdate -radix unsigned /sl_processor_tb/DUT/sl_core_1/cp_addr_o
add wave -noupdate -radix hexadecimal /sl_processor_tb/DUT/sl_core_1/cp_din_i
add wave -noupdate /sl_processor_tb/DUT/sl_core_1/ext_mem_addr_o
add wave -noupdate -radix hexadecimal /sl_processor_tb/DUT/sl_core_1/ext_mem_dout_o
add wave -noupdate -radix hexadecimal /sl_processor_tb/DUT/sl_core_1/ext_mem_din_i
add wave -noupdate /sl_processor_tb/DUT/sl_core_1/ext_mem_rw_o
add wave -noupdate /sl_processor_tb/DUT/sl_core_1/ext_mem_en_o
add wave -noupdate /sl_processor_tb/DUT/sl_core_1/ext_mem_stall_i
add wave -noupdate -radix unsigned /sl_processor_tb/DUT/sl_core_1/rp0_addr_o
add wave -noupdate -radix hexadecimal /sl_processor_tb/DUT/sl_core_1/rp0_din_i
add wave -noupdate -radix unsigned /sl_processor_tb/DUT/sl_core_1/rp1_addr_o
add wave -noupdate -radix hexadecimal /sl_processor_tb/DUT/sl_core_1/rp1_din_i
add wave -noupdate -radix unsigned /sl_processor_tb/DUT/sl_core_1/wp_addr_o
add wave -noupdate -radix hexadecimal -childformat {{/sl_processor_tb/DUT/sl_core_1/wp_dout_o(31) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(30) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(29) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(28) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(27) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(26) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(25) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(24) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(23) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(22) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(21) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(20) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(19) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(18) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(17) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(16) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(15) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(14) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(13) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(12) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(11) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(10) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(9) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(8) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(7) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(6) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(5) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(4) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(3) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(2) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(1) -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(0) -radix hexadecimal}} -subitemconfig {/sl_processor_tb/DUT/sl_core_1/wp_dout_o(31) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(30) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(29) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(28) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(27) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(26) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(25) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(24) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(23) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(22) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(21) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(20) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(19) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(18) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(17) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(16) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(15) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(14) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(13) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(12) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(11) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(10) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(9) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(8) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(7) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(6) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(5) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(4) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(3) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(2) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(1) {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/wp_dout_o(0) {-height 16 -radix hexadecimal}} /sl_processor_tb/DUT/sl_core_1/wp_dout_o
add wave -noupdate /sl_processor_tb/DUT/sl_core_1/wp_we_o
add wave -noupdate -expand -subitemconfig {/sl_processor_tb/DUT/sl_core_1/proc.state {-height 16 -childformat {{/sl_processor_tb/DUT/sl_core_1/proc.state.pc -radix unsigned} {/sl_processor_tb/DUT/sl_core_1/proc.state.addr -radix unsigned} {/sl_processor_tb/DUT/sl_core_1/proc.state.irs -radix unsigned} {/sl_processor_tb/DUT/sl_core_1/proc.state.loop_count -radix unsigned} {/sl_processor_tb/DUT/sl_core_1/proc.state.result -radix hexadecimal}} -expand} /sl_processor_tb/DUT/sl_core_1/proc.state.pc {-radix unsigned} /sl_processor_tb/DUT/sl_core_1/proc.state.addr {-height 16 -radix unsigned} /sl_processor_tb/DUT/sl_core_1/proc.state.irs {-height 16 -radix unsigned} /sl_processor_tb/DUT/sl_core_1/proc.state.loop_count {-radix unsigned} /sl_processor_tb/DUT/sl_core_1/proc.state.result {-radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/proc.fetch {-childformat {{/sl_processor_tb/DUT/sl_core_1/proc.fetch.data -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/proc.fetch.pc -radix unsigned}} -expand} /sl_processor_tb/DUT/sl_core_1/proc.fetch.data {-radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/proc.fetch.pc {-radix unsigned} /sl_processor_tb/DUT/sl_core_1/proc.dec {-height 16 -childformat {{/sl_processor_tb/DUT/sl_core_1/proc.dec.irs_addr -radix unsigned} {/sl_processor_tb/DUT/sl_core_1/proc.dec.cur_pc -radix unsigned} {/sl_processor_tb/DUT/sl_core_1/proc.dec.jmp_target_pc -radix unsigned}} -expand} /sl_processor_tb/DUT/sl_core_1/proc.dec.irs_addr {-height 16 -radix unsigned} /sl_processor_tb/DUT/sl_core_1/proc.dec.cur_pc {-height 16 -radix unsigned} /sl_processor_tb/DUT/sl_core_1/proc.dec.jmp_target_pc {-height 16 -radix unsigned} /sl_processor_tb/DUT/sl_core_1/proc.decex {-childformat {{/sl_processor_tb/DUT/sl_core_1/proc.decex.a -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/proc.decex.b -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/proc.decex.mem0 -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/proc.decex.mem1 -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/proc.decex.memX -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/proc.decex.wr_addr -radix unsigned}} -expand} /sl_processor_tb/DUT/sl_core_1/proc.decex.a {-radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/proc.decex.b {-radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/proc.decex.mem0 {-radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/proc.decex.mem1 {-radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/proc.decex.memX {-radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/proc.decex.wr_addr {-radix unsigned}} /sl_processor_tb/DUT/sl_core_1/proc
add wave -noupdate /sl_processor_tb/DUT/sl_core_1/mem1_next
add wave -noupdate /sl_processor_tb/DUT/sl_core_1/mem2_next
add wave -noupdate -childformat {{/sl_processor_tb/DUT/sl_core_1/dec_next.irs_addr -radix unsigned} {/sl_processor_tb/DUT/sl_core_1/dec_next.cur_pc -radix unsigned} {/sl_processor_tb/DUT/sl_core_1/dec_next.jmp_target_pc -radix unsigned}} -expand -subitemconfig {/sl_processor_tb/DUT/sl_core_1/dec_next.irs_addr {-height 16 -radix unsigned} /sl_processor_tb/DUT/sl_core_1/dec_next.cur_pc {-height 16 -radix unsigned} /sl_processor_tb/DUT/sl_core_1/dec_next.jmp_target_pc {-height 16 -radix unsigned}} /sl_processor_tb/DUT/sl_core_1/dec_next
add wave -noupdate -childformat {{/sl_processor_tb/DUT/sl_core_1/decex_next.a -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/decex_next.b -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/decex_next.mem0 -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/decex_next.mem1 -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/decex_next.memX -radix hexadecimal} {/sl_processor_tb/DUT/sl_core_1/decex_next.wr_addr -radix unsigned} {/sl_processor_tb/DUT/sl_core_1/decex_next.load_data -radix hexadecimal}} -expand -subitemconfig {/sl_processor_tb/DUT/sl_core_1/decex_next.a {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/decex_next.b {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/decex_next.mem0 {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/decex_next.mem1 {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/decex_next.memX {-height 16 -radix hexadecimal} /sl_processor_tb/DUT/sl_core_1/decex_next.wr_addr {-height 16 -radix unsigned} /sl_processor_tb/DUT/sl_core_1/decex_next.load_data {-height 16 -radix hexadecimal}} /sl_processor_tb/DUT/sl_core_1/decex_next
add wave -noupdate -expand /sl_processor_tb/DUT/sl_core_1/exec_next
add wave -noupdate -expand /sl_processor_tb/DUT/sl_core_1/ctrl_next
add wave -noupdate /sl_processor_tb/DUT/sl_core_1/state_next
add wave -noupdate -radix unsigned /sl_processor_tb/DUT/sl_core_1/rp0_addr
add wave -noupdate -radix unsigned /sl_processor_tb/DUT/sl_core_1/rp1_addr
add wave -noupdate -divider Unit
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/clk_i
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/reset_n_i
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/cmd_i
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/ready_o
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/start_i
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/regA_i
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/regB_i
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/result_o
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/cmp_gt_o
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/cmp_z_o
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/complete_o
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/units_start
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/units_ready
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/units_complete
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/units_result
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/i
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/j
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/regA
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/regB
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/cmp_le
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/result
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/active_unit
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/sign_ext
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/raw_result
add wave -noupdate /sl_processor_tb/DUT/qfp_unit_1/result_zero
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {123068 ps} 0}
quietly wave cursor active 1
configure wave -namecolwidth 206
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
configure wave -timelineunits ps
update
WaveRestoreZoom {0 ps} {324313 ps}
