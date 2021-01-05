cmake_minimum_required(VERSION 3.10)

project(ProcessorSim)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR} *.cpp)
file(GLOB_RECURSE HEADER ${PROJECT_SOURCE_DIR} *.h)
list(APPEND SRC ${HEADER})
list(FILTER SRC EXCLUDE REGEX ".*CMake.*")

add_library(ProcessorSim STATIC ${SRC})
target_include_directories(ProcessorSim PUBLIC .)

