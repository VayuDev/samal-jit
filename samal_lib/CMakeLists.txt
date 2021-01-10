cmake_minimum_required(VERSION 3.0)

project(samal_lib)
set (CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -fno-strict-aliasing -Dx86_64_BIT_MODE -DSAMAL_ENABLE_JIT")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-strict-aliasing -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -march=native -mtune=native -fno-strict-aliasing")
set(CMAKE_C_FLAGS_RELEASE "-O3 -g -march=native -mtune=native -fno-strict-aliasing")

file(GLOB_RECURSE HEADERS
        "${PROJECT_SOURCE_DIR}/include/*.hpp")

file(GLOB_RECURSE SOURCES
        "${PROJECT_SOURCE_DIR}/lib/*.cpp")

include_directories(include)
include_directories(../peg_parser/include)
include_directories(../third_party/xbyak/xbyak)
add_library(samal_lib STATIC ${SOURCES} ${HEADERS})
target_link_libraries(samal_lib EasyIterator peg_parser xbyak)
