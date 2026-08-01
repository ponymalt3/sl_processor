add_custom_target(sim_system
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run.py
        --build-dir ${CMAKE_BINARY_DIR}/sim/sim_system
        --gen-code $<TARGET_FILE:gen_code>
        --prog ${CMAKE_CURRENT_SOURCE_DIR}/prog.rt
        $<$<NOT:$<BOOL:${SIM_WAVES}>>:--no-waves>
    COMMENT "Running sl_system top-entity cocotb tests"
    USES_TERMINAL
)
add_dependencies(sim_system gen_code)
