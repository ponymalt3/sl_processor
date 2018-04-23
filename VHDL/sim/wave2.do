onerror {resume}
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000001
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000002
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000003
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000004
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000005
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000006
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000007
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000008
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000009
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000010
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000011
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000012
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000013
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000014
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000015
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000016
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000017
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000018
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000019
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000020
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000021
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000022
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000023
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000024
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000025
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000026
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000027
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000028
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000029
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000030
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000031
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000032
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000033
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000034
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000035
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000036
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000037
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000038
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000039
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000040
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000041
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000042
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000043
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000044
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000045
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000046
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000047
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000048
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000049
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000050
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000051
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000052
quietly virtual function -install /sl_test_tb -env /sl_test_tb { 0.1} virtual_000053
quietly WaveActivateNextPane {} 0
add wave -noupdate /sl_test_tb/clk
add wave -noupdate /sl_test_tb/reset_n
add wave -noupdate /sl_test_tb/sl_clk
add wave -noupdate -radix unsigned /sl_test_tb/code_addr
add wave -noupdate /sl_test_tb/code_data
add wave -noupdate -radix unsigned /sl_test_tb/mem_addr
add wave -noupdate -radix hexadecimal /sl_test_tb/mem_din
add wave -noupdate -radix hexadecimal /sl_test_tb/mem_dout
add wave -noupdate /sl_test_tb/mem_we
add wave -noupdate /sl_test_tb/mem_complete
add wave -noupdate -radix unsigned /sl_test_tb/executed_addr
add wave -noupdate -radix unsigned /sl_test_tb/ext_mem_addr
add wave -noupdate -radix hexadecimal /sl_test_tb/ext_mem_dout
add wave -noupdate -radix hexadecimal /sl_test_tb/ext_mem_din
add wave -noupdate /sl_test_tb/ext_mem_rw
add wave -noupdate /sl_test_tb/ext_mem_en
add wave -noupdate /sl_test_tb/ext_mem_stall
add wave -noupdate -childformat {{/sl_test_tb/ext_mem(1001) -radix hexadecimal} {/sl_test_tb/ext_mem(1000) -radix hexadecimal}} -subitemconfig {/sl_test_tb/ext_mem(1001) {-height 16 -radix hexadecimal} /sl_test_tb/ext_mem(1000) {-height 16 -radix hexadecimal}} /sl_test_tb/ext_mem
add wave -noupdate -divider DUT
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/clk_i
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/reset_n_i
add wave -noupdate /sl_test_tb/sl_processor_1/reset_core_n_i
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/alu_en_o
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/alu_cmd_o
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/sl_core_1/alu_op_a_o
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/sl_core_1/alu_op_b_o
add wave -noupdate -childformat {{/sl_test_tb/sl_processor_1/sl_core_1/alu_i.result -radix hexadecimal} {/sl_test_tb/sl_processor_1/sl_core_1/alu_i.int_result -radix hexadecimal}} -expand -subitemconfig {/sl_test_tb/sl_processor_1/sl_core_1/alu_i.result {-height 16 -radix hexadecimal} /sl_test_tb/sl_processor_1/sl_core_1/alu_i.int_result {-height 16 -radix hexadecimal}} /sl_test_tb/sl_processor_1/sl_core_1/alu_i
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/sl_core_1/cp_addr_o
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/sl_core_1/cp_din_i
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/sl_core_1/ext_mem_addr_o
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/sl_core_1/ext_mem_dout_o
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/sl_core_1/ext_mem_din_i
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/ext_mem_rw_o
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/ext_mem_en_o
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/ext_mem_stall_i
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/sl_core_1/rp0_addr_o
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/sl_core_1/rp0_din_i
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/sl_core_1/rp1_addr_o
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/sl_core_1/rp1_din_i
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/rp_stall_o
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/sl_core_1/wp_addr_o
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/sl_core_1/wp_dout_o
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/wp_we_o
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/executed_addr_o
add wave -noupdate -childformat {{/sl_test_tb/sl_processor_1/sl_core_1/proc.fetch -radix hexadecimal}} -expand -subitemconfig {/sl_test_tb/sl_processor_1/sl_core_1/proc.state {-height 16 -childformat {{/sl_test_tb/sl_processor_1/sl_core_1/proc.state.pc -radix unsigned} {/sl_test_tb/sl_processor_1/sl_core_1/proc.state.irs -radix unsigned} {/sl_test_tb/sl_processor_1/sl_core_1/proc.state.loop_count -radix unsigned} {/sl_test_tb/sl_processor_1/sl_core_1/proc.state.result -radix hexadecimal}} -expand} /sl_test_tb/sl_processor_1/sl_core_1/proc.state.pc {-height 16 -radix unsigned} /sl_test_tb/sl_processor_1/sl_core_1/proc.state.addr {-height 16 -childformat {{/sl_test_tb/sl_processor_1/sl_core_1/proc.state.addr(1) -radix unsigned} {/sl_test_tb/sl_processor_1/sl_core_1/proc.state.addr(0) -radix unsigned}} -expand} /sl_test_tb/sl_processor_1/sl_core_1/proc.state.addr(1) {-height 16 -radix unsigned} /sl_test_tb/sl_processor_1/sl_core_1/proc.state.addr(0) {-height 16 -radix unsigned} /sl_test_tb/sl_processor_1/sl_core_1/proc.state.irs {-height 16 -radix unsigned} /sl_test_tb/sl_processor_1/sl_core_1/proc.state.loop_count {-height 16 -radix unsigned} /sl_test_tb/sl_processor_1/sl_core_1/proc.state.result {-height 16 -radix hexadecimal} /sl_test_tb/sl_processor_1/sl_core_1/proc.fetch {-height 16 -radix hexadecimal} /sl_test_tb/sl_processor_1/sl_core_1/proc.decex {-height 16 -childformat {{/sl_test_tb/sl_processor_1/sl_core_1/proc.decex.memX -radix hexadecimal} {/sl_test_tb/sl_processor_1/sl_core_1/proc.decex.wr_addr -radix unsigned}} -expand} /sl_test_tb/sl_processor_1/sl_core_1/proc.decex.memX {-height 16 -radix hexadecimal} /sl_test_tb/sl_processor_1/sl_core_1/proc.decex.wr_addr {-height 16 -radix unsigned}} /sl_test_tb/sl_processor_1/sl_core_1/proc
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/fetch_next
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/mem1_next
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/mem2_next
add wave -noupdate -childformat {{/sl_test_tb/sl_processor_1/sl_core_1/dec_next.irs_addr -radix unsigned}} -expand -subitemconfig {/sl_test_tb/sl_processor_1/sl_core_1/dec_next.irs_addr {-height 16 -radix unsigned}} /sl_test_tb/sl_processor_1/sl_core_1/dec_next
add wave -noupdate -childformat {{/sl_test_tb/sl_processor_1/sl_core_1/decex_next.a -radix hexadecimal} {/sl_test_tb/sl_processor_1/sl_core_1/decex_next.b -radix hexadecimal} {/sl_test_tb/sl_processor_1/sl_core_1/decex_next.mem0 -radix hexadecimal} {/sl_test_tb/sl_processor_1/sl_core_1/decex_next.mem1 -radix hexadecimal} {/sl_test_tb/sl_processor_1/sl_core_1/decex_next.memX -radix hexadecimal} {/sl_test_tb/sl_processor_1/sl_core_1/decex_next.wr_addr -radix unsigned}} -expand -subitemconfig {/sl_test_tb/sl_processor_1/sl_core_1/decex_next.a {-height 16 -radix hexadecimal} /sl_test_tb/sl_processor_1/sl_core_1/decex_next.b {-height 16 -radix hexadecimal} /sl_test_tb/sl_processor_1/sl_core_1/decex_next.mem0 {-height 16 -radix hexadecimal} /sl_test_tb/sl_processor_1/sl_core_1/decex_next.mem1 {-height 16 -radix hexadecimal} /sl_test_tb/sl_processor_1/sl_core_1/decex_next.memX {-height 16 -radix hexadecimal} /sl_test_tb/sl_processor_1/sl_core_1/decex_next.wr_addr {-height 16 -radix unsigned}} /sl_test_tb/sl_processor_1/sl_core_1/decex_next
add wave -noupdate -childformat {{/sl_test_tb/sl_processor_1/sl_core_1/exec_next.result -radix hexadecimal} {/sl_test_tb/sl_processor_1/sl_core_1/exec_next.int_result -radix hexadecimal}} -expand -subitemconfig {/sl_test_tb/sl_processor_1/sl_core_1/exec_next.result {-height 16 -radix hexadecimal} /sl_test_tb/sl_processor_1/sl_core_1/exec_next.int_result {-height 16 -radix hexadecimal}} /sl_test_tb/sl_processor_1/sl_core_1/exec_next
add wave -noupdate -expand /sl_test_tb/sl_processor_1/sl_core_1/ctrl_next
add wave -noupdate /sl_test_tb/sl_processor_1/sl_core_1/state_next
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/sl_core_1/rp0_addr
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/sl_core_1/rp1_addr
add wave -noupdate -divider mem
add wave -noupdate /sl_test_tb/sl_processor_1/multi_port_mem_1/clk_i
add wave -noupdate /sl_test_tb/sl_processor_1/multi_port_mem_1/reset_n_i
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/multi_port_mem_1/wport_addr_i
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/multi_port_mem_1/wport_din_i
add wave -noupdate /sl_test_tb/sl_processor_1/multi_port_mem_1/wport_we_i
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/multi_port_mem_1/rport0_addr_i
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/multi_port_mem_1/rport0_dout_o
add wave -noupdate /sl_test_tb/sl_processor_1/multi_port_mem_1/rport0_en_i
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/multi_port_mem_1/rwport_addr_i
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/multi_port_mem_1/rwport_din_i
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/multi_port_mem_1/rwport_dout_o
add wave -noupdate /sl_test_tb/sl_processor_1/multi_port_mem_1/rwport_we_i
add wave -noupdate /sl_test_tb/sl_processor_1/multi_port_mem_1/rwport_complete_o
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/multi_port_mem_1/rport1_addr_i
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/multi_port_mem_1/rport1_dout_o
add wave -noupdate /sl_test_tb/sl_processor_1/multi_port_mem_1/rport_stall_i
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/multi_port_mem_1/p0_addr
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/multi_port_mem_1/p0_din
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/multi_port_mem_1/p0_dout
add wave -noupdate /sl_test_tb/sl_processor_1/multi_port_mem_1/p0_we
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/multi_port_mem_1/p1_addr
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/multi_port_mem_1/p1_din
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/multi_port_mem_1/p1_dout
add wave -noupdate /sl_test_tb/sl_processor_1/multi_port_mem_1/p1_we
add wave -noupdate /sl_test_tb/sl_processor_1/multi_port_mem_1/p0_en
add wave -noupdate /sl_test_tb/sl_processor_1/multi_port_mem_1/p1_en
add wave -noupdate /sl_test_tb/sl_processor_1/multi_port_mem_1/state
add wave -noupdate -radix hexadecimal /sl_test_tb/sl_processor_1/multi_port_mem_1/m9k_1/mem
add wave -noupdate -divider xxx
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/clk_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/reset_n_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/cmd_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/start_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/ready_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/regA_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/regB_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/complete_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/result_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/cmp_le_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p1_gt
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p1_fmt
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p1_mant_a
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p1_mant_b
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p1_cy
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p1_op_a
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p1_op_b
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p1_clag1
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p1_clag2
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p1_cmp_gt
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p1_is_add
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p1_exp1
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_complete
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_gt
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_fmt
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_cy
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_op_a
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_op_b
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_clag1
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_clag2
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_is_add
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_exp1
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_exp2
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_result
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_sign
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp32_add_1/p2_cmp_gt
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/cmp_gt_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/cmp_z_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/clk_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/reset_n_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/raw_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/result_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/result_zero_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/is_ext
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/err
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/mant
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/full_mant
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/exp
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/rounding
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/rd_mant
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/rounding_err
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/zero
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/qfp_norm_1/norm
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/clk_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/reset_n_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/cmd_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/ready_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/start_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/regA_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/regB_i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/result_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/cmp_gt_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/cmp_z_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/complete_o
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/units_start
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/units_ready
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/units_complete
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/units_result
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/i
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/j
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/regA
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/regB
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/cmp_le
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/result
add wave -noupdate -radix unsigned /sl_test_tb/sl_processor_1/qfp_unit_1/active_unit
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/sign_ext
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/cmp_z
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/raw_result
add wave -noupdate /sl_test_tb/sl_processor_1/qfp_unit_1/result_zero
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 2} {634383 ps} 1} {{Cursor 3} {571295 ps} 0}
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
configure wave -timelineunits ps
update
WaveRestoreZoom {217355 ps} {765195 ps}
bookmark add wave bookmark0 {{75510164 ps} {76605844 ps}} 0
