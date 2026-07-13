onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate /top_tb/DUT/clock_50
add wave -noupdate /top_tb/DUT/sw
add wave -noupdate /top_tb/DUT/key
add wave -noupdate /top_tb/DUT/led
add wave -noupdate /top_tb/DUT/dram_addr
add wave -noupdate /top_tb/DUT/dram_dq
add wave -noupdate /top_tb/DUT/gpio_0
add wave -noupdate /top_tb/DUT/gpio_1
add wave -noupdate /top_tb/DUT/gpio_2
add wave -noupdate /top_tb/DUT/wbs_in
add wave -noupdate /top_tb/DUT/wbs_out
add wave -noupdate /top_tb/DUT/wb_en
add wave -noupdate /top_tb/DUT/wb_re
add wave -noupdate /top_tb/DUT/wb_we
add wave -noupdate -radix hexadecimal /top_tb/DUT/wb_dout
add wave -noupdate /top_tb/DUT/wb_addr
add wave -noupdate /top_tb/DUT/wb_complete
add wave -noupdate -radix unsigned /top_tb/DUT/addr
add wave -noupdate -radix unsigned /top_tb/DUT/len
add wave -noupdate /top_tb/DUT/state
add wave -noupdate -radix unsigned /top_tb/DUT/cmd
add wave -noupdate /top_tb/DUT/rdata
add wave -noupdate /top_tb/DUT/rstate
add wave -noupdate -radix hexadecimal /top_tb/DUT/wdata
add wave -noupdate /top_tb/DUT/wstate
add wave -noupdate /top_tb/DUT/core_en
add wave -noupdate /top_tb/DUT/core_reset
add wave -noupdate /top_tb/DUT/uart_cr
add wave -noupdate /top_tb/DUT/uart_cw
add wave -noupdate /top_tb/DUT/uart_we
add wave -noupdate -radix unsigned /top_tb/DUT/uart_din
add wave -noupdate -radix hexadecimal /top_tb/DUT/uart_dout
add wave -noupdate /top_tb/DUT/uart_rxd
add wave -noupdate /top_tb/DUT/uart_txd
add wave -noupdate /top_tb/DUT/reset_n
add wave -noupdate /top_tb/DUT/clk
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {2539842913 ps} 0}
quietly wave cursor active 1
configure wave -namecolwidth 295
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
WaveRestoreZoom {815185920 ps} {4891115520 ps}
