file(GLOB_RECURSE SRC *.cpp *.h)
list(FILTER SRC EXCLUDE REGEX ".*CMake.*")

add_library(Assembler STATIC ${SRC})
target_include_directories(Assembler
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/VHDL/qfp32/trunk
)
target_link_libraries(Assembler PRIVATE ProcessorSim)
