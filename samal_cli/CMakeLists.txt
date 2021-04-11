cmake_minimum_required(VERSION 3.0)

project(samal)
set (CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-strict-aliasing -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fno-strict-aliasing -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g -march=native -mtune=native -fno-strict-aliasing")

file(GLOB_RECURSE HEADERS
        "${PROJECT_SOURCE_DIR}/include/*.hpp")

file(GLOB_RECURSE SOURCES
        "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(samal ${SOURCES} ${HEADERS})

include_directories(../peg_parser/include)
include_directories(../samal_lib/include)
include_directories(include)
target_link_libraries(samal samal_lib -lstdc++)

if(SAMAL_ENABLE_GFX_CAIRO)
    target_link_libraries(samal ${CAIROMM_LIBRARIES})
endif()

