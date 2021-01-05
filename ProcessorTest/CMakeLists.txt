cmake_minimum_required(VERSION 3.10)

project(ProcessorTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR} *.cpp)
#file(GLOB_RECURSE HEADER ${PROJECT_SOURCE_DIR} *.h)
#list (APPEND SRC ${HEADER})
list(FILTER SRC EXCLUDE REGEX ".*CMake.*")
set(TEST_SRC ${TEST_SRC} ${SRC} PARENT_SCOPE) 

#add_library(ProcessorTest "${SRC}")
#target_include_directories(ProcessorTest PUBLIC .)

#add_test (NAME ProcessorTest COMMAND ProcessorTest)
