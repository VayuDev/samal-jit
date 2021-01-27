cmake_minimum_required(VERSION 3.0)

project(samal)

option(SAMAL_ENABLE_JIT "Build samal jit" OFF)
if(SAMAL_ENABLE_JIT)
    add_subdirectory(third_party/xbyak)
endif()

option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." ON)
if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options (-fcolor-diagnostics)
    endif ()
endif ()

add_subdirectory(peg_parser)
add_subdirectory(samal_lib)
add_subdirectory(samal_cli)
add_subdirectory(tests)