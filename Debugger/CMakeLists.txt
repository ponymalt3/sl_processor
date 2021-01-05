cmake_minimum_required(VERSION 3.10)

project(Debugger)

include_directories(../qfp32/trunk ../ProcessorSim ..)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR} *.cpp)
file(GLOB_RECURSE HEADER ${PROJECT_SOURCE_DIR} *.h)
list (APPEND SRC ${HEADER})
list(FILTER SRC EXCLUDE REGEX ".*CMake.*")

add_subdirectory(../ProcessorSim ${CMAKE_CURRENT_BINARY_DIR}/x)
add_subdirectory(../Assembler ${CMAKE_CURRENT_BINARY_DIR}/xx)

add_executable(Debugger "${SRC}")
target_link_libraries(Debugger PUBLIC ProcessorSim Assembler)
