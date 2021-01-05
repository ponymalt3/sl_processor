cmake_minimum_required(VERSION 3.10)

project(ProcessorAsmTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR} *.cpp)
#file(GLOB_RECURSE HEADER ${PROJECT_SOURCE_DIR} *.h)
#list (APPEND SRC ${HEADER})
list(FILTER SRC EXCLUDE REGEX ".*CMake.*")
set(TEST_SRC ${TEST_SRC} ${SRC} PARENT_SCOPE) 

#add_library(ProcessorAsmTest "${SRC}")
#target_link_libraries(ProcessorAsmTest ProcessorLib RTAsm)

#add_test (NAME ProcessorAsmTest COMMAND ProcessorAsmTest)
