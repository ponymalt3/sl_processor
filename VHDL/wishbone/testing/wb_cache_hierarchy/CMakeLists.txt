add_custom_target(sim_wb_cache_hierarchy
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run.py
        --build-dir ${CMAKE_BINARY_DIR}/sim/sim_wb_cache_hierarchy
        $<$<NOT:$<BOOL:${SIM_WAVES}>>:--no-waves>
    COMMENT "Running multi-level wb_cache hierarchy stress tests"
    USES_TERMINAL
)
