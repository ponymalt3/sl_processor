cmake_minimum_required(VERSION 3.10)
project(SLProcessor)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(ProcessorSim)
add_subdirectory(Assembler)
add_subdirectory(Debugger)
add_subdirectory(testing)

# ---------------------------------------------------------------------------
# Cocotb simulation targets
# ---------------------------------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# FST waveforms written to build/sim/<target>/*.fst — open with GTKWave.
# Disable: cmake -DSIM_WAVES=OFF
option(SIM_WAVES "Generate FST waveform files (open with GTKWave)" ON)

add_subdirectory(VHDL/sl_processor/testing)
add_subdirectory(VHDL/wishbone/testing/wb_cache)
add_subdirectory(VHDL/wishbone/testing/wb_interconnect)
add_subdirectory(VHDL/qfp32/Test)

# cmake --build . --target sim_all  runs all four in parallel
add_custom_target(sim_all)
add_dependencies(sim_all sim_processor sim_wb_cache sim_wb_interconnect sim_qfp32)
