cmake_minimum_required(VERSION 3.0)

project(samal_tests)
set (CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-strict-aliasing -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -march=native -mtune=native -fno-strict-aliasing")
set(CMAKE_C_FLAGS_RELEASE "-O3 -g -march=native -mtune=native -fno-strict-aliasing")
file(GLOB_RECURSE SOURCES
        "${PROJECT_SOURCE_DIR}/*.cpp")

include_directories(include)
include_directories(../third_party)
include_directories(../peg_parser/include)
include_directories(../samal_lib/include)

add_executable(samal_tests ${SOURCES} ${HEADERS})
target_link_libraries(samal_tests samal_lib peg_parser -lstdc++ m)

enable_testing()
add_test("PEG_Parser_Test" samal_tests)
