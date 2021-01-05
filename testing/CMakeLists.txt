cmake_minimum_required(VERSION 3.10)

project(Test)

include_directories(../qfp32/trunk ../mtest ../ProcessorSim ..)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(../ProcessorSim ${CMAKE_CURRENT_BINARY_DIR}/x)
add_subdirectory(../Assembler ${CMAKE_CURRENT_BINARY_DIR}/xx)

file(GLOB_RECURSE SRC *.cpp *.h)
list(APPEND SRC "main.cpp")
list(FILTER SRC EXCLUDE REGEX ".*CMake.*")

message(${SRC})

add_executable(Test ${SRC})
target_link_libraries(Test PUBLIC ProcessorSim Assembler)

add_custom_command(TARGET Test
                   COMMENT "Run tests"
                   POST_BUILD COMMAND ./Test && cp test.vector ../VHDL/testing/sim)
