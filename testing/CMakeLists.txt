file(GLOB_RECURSE SRC *.cpp *.h)
list(FILTER SRC EXCLUDE REGEX ".*CMake.*")

add_executable(Test ${SRC})
target_include_directories(Test PRIVATE
    ${CMAKE_SOURCE_DIR}/VHDL/qfp32
    ${CMAKE_SOURCE_DIR}/mtest
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(Test PRIVATE ProcessorSim Assembler)

add_custom_command(TARGET Test POST_BUILD
    COMMENT "Run C++ tests"
    COMMAND $<TARGET_FILE:Test>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
)
