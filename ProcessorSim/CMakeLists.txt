file(GLOB_RECURSE SRC *.cpp *.h)
list(FILTER SRC EXCLUDE REGEX ".*CMake.*")

add_library(ProcessorSim STATIC ${SRC})
target_include_directories(ProcessorSim
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/VHDL/qfp32
)
