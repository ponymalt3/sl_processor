add_executable(gen_code gen_code.cpp)
target_include_directories(gen_code PRIVATE
    ${CMAKE_SOURCE_DIR}/VHDL/qfp32
    ${CMAKE_SOURCE_DIR}/mtest
    ${CMAKE_SOURCE_DIR}/testing/Asm
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(gen_code PRIVATE ProcessorSim Assembler)

add_custom_target(sim_cluster
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run.py
        --build-dir ${CMAKE_BINARY_DIR}/sim/sim_cluster
        --gen-code $<TARGET_FILE:gen_code>
        --prog ${CMAKE_CURRENT_SOURCE_DIR}/prog_resolved
        $<$<NOT:$<BOOL:${SIM_WAVES}>>:--no-waves>
    COMMENT "Running sl_cluster cocotb tests"
    USES_TERMINAL
)
add_dependencies(sim_cluster gen_code)
