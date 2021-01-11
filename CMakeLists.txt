cmake_minimum_required(VERSION 3.0)

project(samal)

option(SAMAL_ENABLE_JIT "Build samal jit" OFF)
if(SAMAL_ENABLE_JIT)
    add_subdirectory(third_party/xbyak)
endif()
add_subdirectory(peg_parser)
add_subdirectory(samal_lib)
add_subdirectory(samal_cli)
add_subdirectory(tests)
