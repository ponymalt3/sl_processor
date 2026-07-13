set_global_assignment -entity "memclk" -library "lib_memclk" -name IP_TOOL_NAME "Qsys"
set_global_assignment -entity "memclk" -library "lib_memclk" -name IP_TOOL_VERSION "17.1"
set_global_assignment -entity "memclk" -library "lib_memclk" -name IP_TOOL_ENV "Qsys"
set_global_assignment -library "lib_memclk" -name SPD_FILE [file join $::quartus(sip_path) "../memclk.spd"]
set_global_assignment -library "lib_memclk" -name MISC_FILE [file join $::quartus(sip_path) "../../memclk.qsys"]

set_global_assignment -library "lib_memclk" -name MISC_FILE [file join $::quartus(sip_path) "memclk.vhd"]
set_global_assignment -library "lib_memclk" -name MISC_FILE [file join $::quartus(sip_path) "submodules/memclk_altclkctrl_0.v"]
