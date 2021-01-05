cmake_minimum_required(VERSION 3.10)

project(Test)

include_directories(mtest qfp32/trunk ProcessorLib . ProcessorTest ProcessorAsmTest)

add_subdirectory(ProcessorLib)
add_subdirectory(RTAsm)

set(TEST_SRC "main.cpp")

add_subdirectory(ProcessorTest)
add_subdirectory(ProcessorAsmTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message("${TEST_SRC}")

add_executable(Test ${TEST_SRC})
target_link_libraries(Test PUBLIC ProcessorLib RTAsm)

add_custom_command(TARGET Test
                   COMMENT "Run tests"
                   POST_BUILD COMMAND ./Test && cp test.vector ./VHDL/functional_testing/sim
)
