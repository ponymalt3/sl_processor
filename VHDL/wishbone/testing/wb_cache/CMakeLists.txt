add_custom_target(sim_wb_cache
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run.py
        --build-dir ${CMAKE_BINARY_DIR}/sim/sim_wb_cache
        $<$<NOT:$<BOOL:${SIM_WAVES}>>:--no-waves>
    COMMENT "Running wb_cache cocotb tests"
    USES_TERMINAL
)
