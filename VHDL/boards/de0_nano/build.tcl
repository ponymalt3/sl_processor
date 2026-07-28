# Creates the Quartus project and all its assignments, then runs the full
# compile flow (map/fit/asm/sta). This script is the source of truth for
# the project setup -- no hand-maintained .qsf is checked in; project_new
# below generates one as a build artifact (see synth.sh for the wrapper).
#
# Pin locations are Terasic DE0-Nano board facts (Cyclone IV EP4CE22F17C6),
# independently sourced from the official Terasic DE0-Nano User Manual.
#
# Usage: quartus_sh -t build.tcl   (run from this directory)

load_package flow

project_new de0_nano_top -overwrite

set_global_assignment -name FAMILY "Cyclone IV E"
set_global_assignment -name DEVICE EP4CE22F17C6
set_global_assignment -name TOP_LEVEL_ENTITY de0_nano_top
set_global_assignment -name PROJECT_OUTPUT_DIRECTORY output_files
set_global_assignment -name VHDL_INPUT_VERSION VHDL_2008
set_global_assignment -name MIN_CORE_JUNCTION_TEMP 0
set_global_assignment -name MAX_CORE_JUNCTION_TEMP 85
set_global_assignment -name ERROR_CHECK_FREQUENCY_DIVISOR 1

# ---------------------------------------------------------------------------
# Sources, in dependency order (mirrors VHDL/sl_processor/testing/cluster/run.py)
# ---------------------------------------------------------------------------
foreach f {
    ../../wishbone/wishbone_p.vhd
    ../../sl_misc.vhd
    ../../sl_dpram.vhd
    ../../wishbone/wb_ixs_decode.vhd
    ../../wishbone/wb_ixs_arbiter.vhd
    ../../wishbone/wb_ixs.vhd
    ../../wishbone/wb_master.vhd
    ../../wishbone/wb_cache.vhd
    ../../wishbone/adapter/wb_cache_adapter.vhd
    ../../wishbone/wb_mem.vhd
    ../../wishbone/adapter/wb_abp_bridge.vhd
    ../../wishbone/wb_sdram.vhd
    ../../top/uart.vhd
    ../../top/wb_debug_ctrl.vhd
    ../../sl_processor/sl_structs_p.vhd
    ../../sl_processor/sl_control.vhd
    ../../sl_processor/sl_dec.vhd
    ../../sl_processor/sl_dec_ex.vhd
    ../../sl_processor/sl_execute.vhd
    ../../sl_processor/sl_state.vhd
    ../../qfp32/cla.vhd
    ../../qfp32/qfp_p.vhd
    ../../qfp32/Units/misc.vhd
    ../../qfp32/Units/add.vhd
    ../../qfp32/Units/mul.vhd
    ../../qfp32/Units/divider.vhd
    ../../qfp32/Units/norm.vhd
    ../../qfp32/Units/recp.vhd
    ../../qfp32/Units/math.vhd
    ../../qfp32/Units/mac.vhd
    ../../qfp32/unit.vhd
    ../../sl_processor/sl_core.vhd
    ../../sl_processor/sl_code_mem.vhd
    ../../sl_processor/sl_processor.vhd
    ../../sl_processor/sl_cluster.vhd
    ../../top/top.vhd
    pll.vhd
    de0_nano_top.vhd
} {
    set_global_assignment -name VHDL_FILE $f
}

set_global_assignment -name SDC_FILE de0_nano_top.sdc

# ---------------------------------------------------------------------------
# Pin assignments (Terasic DE0-Nano User Manual)
# ---------------------------------------------------------------------------
proc pin {signal location} {
    set_location_assignment $location -to $signal
    set_instance_assignment -name IO_STANDARD "3.3-V LVTTL" -to $signal
}

pin CLOCK_50 PIN_R8

pin KEY[0] PIN_J15
pin KEY[1] PIN_E1

pin SW[0] PIN_M1
pin SW[1] PIN_T8
pin SW[2] PIN_B9
pin SW[3] PIN_M15

pin LED[0] PIN_A15
pin LED[1] PIN_A13
pin LED[2] PIN_B13
pin LED[3] PIN_A11
pin LED[4] PIN_D1
pin LED[5] PIN_F3
pin LED[6] PIN_B1
pin LED[7] PIN_L3

pin DRAM_BA[0] PIN_M7
pin DRAM_BA[1] PIN_M6
pin DRAM_DQM[0] PIN_R6
pin DRAM_DQM[1] PIN_T5
pin DRAM_RAS_N PIN_L2
pin DRAM_CAS_N PIN_L1
pin DRAM_CKE PIN_L7
pin DRAM_CLK PIN_R4
pin DRAM_WE_N PIN_C2
pin DRAM_CS_N PIN_P6

pin DRAM_DQ[0] PIN_G2
pin DRAM_DQ[1] PIN_G1
pin DRAM_DQ[2] PIN_L8
pin DRAM_DQ[3] PIN_K5
pin DRAM_DQ[4] PIN_K2
pin DRAM_DQ[5] PIN_J2
pin DRAM_DQ[6] PIN_J1
pin DRAM_DQ[7] PIN_R7
pin DRAM_DQ[8] PIN_T4
pin DRAM_DQ[9] PIN_T2
pin DRAM_DQ[10] PIN_T3
pin DRAM_DQ[11] PIN_R3
pin DRAM_DQ[12] PIN_R5
pin DRAM_DQ[13] PIN_P3
pin DRAM_DQ[14] PIN_N3
pin DRAM_DQ[15] PIN_K1

pin DRAM_ADDR[0] PIN_P2
pin DRAM_ADDR[1] PIN_N5
pin DRAM_ADDR[2] PIN_N6
pin DRAM_ADDR[3] PIN_M8
pin DRAM_ADDR[4] PIN_P8
pin DRAM_ADDR[5] PIN_T7
pin DRAM_ADDR[6] PIN_N8
pin DRAM_ADDR[7] PIN_T6
pin DRAM_ADDR[8] PIN_R1
pin DRAM_ADDR[9] PIN_P1
pin DRAM_ADDR[10] PIN_N2
pin DRAM_ADDR[11] PIN_N1
pin DRAM_ADDR[12] PIN_L4

# GPIO_0[0]/[1]: debug UART RxD/TxD
pin GPIO_0[0] PIN_D3
pin GPIO_0[1] PIN_C3

export_assignments

if {[catch {execute_flow -compile} result]} {
    puts "\nResult: $result\n"
    puts "ERROR: Compile flow failed. See report files."
    project_close
    exit 1
}

puts "\nINFO: Compile flow was successful.\n"
project_close
