# Processor integration tests — depends on Test generating test.vector
add_custom_target(sim_processor
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run.py
        --build-dir ${CMAKE_BINARY_DIR}/sim/sim_processor
        --test-vector ${CMAKE_BINARY_DIR}/testing/test.vector
        $<$<NOT:$<BOOL:${SIM_WAVES}>>:--no-waves>
    COMMENT "Running processor integration tests (parallel)"
    USES_TERMINAL
)
add_dependencies(sim_processor Test)
