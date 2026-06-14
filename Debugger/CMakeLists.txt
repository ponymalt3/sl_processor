file(GLOB_RECURSE SRC *.cpp *.h)
list(FILTER SRC EXCLUDE REGEX ".*CMake.*")

add_executable(Debugger ${SRC})
target_include_directories(Debugger PRIVATE
    ${CMAKE_SOURCE_DIR}/qfp32/trunk
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(Debugger PRIVATE ProcessorSim Assembler)
