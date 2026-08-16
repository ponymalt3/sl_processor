add_custom_target(sim_wb_sdram
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run.py
        --build-dir ${CMAKE_BINARY_DIR}/sim/sim_wb_sdram
        $<$<NOT:$<BOOL:${SIM_WAVES}>>:--no-waves>
    COMMENT "Running wb_sdram cocotb tests"
    USES_TERMINAL
)
