file(GLOB_RECURSE SRC *.cpp *.h)
list(FILTER SRC EXCLUDE REGEX ".*CMake.*")

add_executable(Test ${SRC})
target_include_directories(Test PRIVATE
    ${CMAKE_SOURCE_DIR}/qfp32/trunk
    ${CMAKE_SOURCE_DIR}/mtest
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(Test PRIVATE ProcessorSim Assembler)

add_custom_command(TARGET Test POST_BUILD
    COMMENT "Run tests"
    COMMAND $<TARGET_FILE:Test>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/test.vector
        ${CMAKE_SOURCE_DIR}/VHDL/testing/sim/test.vector
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
