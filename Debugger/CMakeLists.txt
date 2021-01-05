cmake_minimum_required(VERSION 3.10)

project(Debugger)

include_directories(../qfp32/trunk ../ProcessorLib ..)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR} *.cpp)
file(GLOB_RECURSE HEADER ${PROJECT_SOURCE_DIR} *.h)
list (APPEND SRC ${HEADER})
list(FILTER SRC EXCLUDE REGEX ".*CMake.*")

add_subdirectory(../ProcessorLib ${CMAKE_CURRENT_BINARY_DIR}/x)
add_subdirectory(../RTAsm ${CMAKE_CURRENT_BINARY_DIR}/xx)

add_executable(Debugger "${SRC}")
target_link_libraries(Debugger PUBLIC ProcessorLib RTAsm)
#target_include_directories(ProcessorTest PUBLIC .)

#add_test (NAME ProcessorTest COMMAND ProcessorTest)
