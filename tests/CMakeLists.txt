cmake_minimum_required(VERSION 3.0)

project(peg_parser_tests)
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

add_executable(peg_parser_tests ${SOURCES} ${HEADERS})
target_link_libraries(peg_parser_tests peg_parser)

enable_testing()
add_test("PEG_Parser_Test" peg_parser_tests)
