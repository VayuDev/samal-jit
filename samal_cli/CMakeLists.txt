cmake_minimum_required(VERSION 3.0)

project(samal)
set (CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-strict-aliasing -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -march=native -mtune=native -fno-strict-aliasing")
set(CMAKE_C_FLAGS_RELEASE "-O3 -g -march=native -mtune=native -fno-strict-aliasing")

if(SAMAL_ENABLE_JIT)
    message(STATUS "Enable JIT")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dx86_64_BIT_MODE -DSAMAL_ENABLE_JIT")
endif()

file(GLOB_RECURSE HEADERS
        "${PROJECT_SOURCE_DIR}/include/*.hpp")

file(GLOB_RECURSE SOURCES
        "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(samal ${SOURCES} ${HEADERS})

include_directories(../peg_parser/include)
include_directories(../samal_lib/include)
include_directories(include)
target_link_libraries(samal samal_lib -lstdc++)
