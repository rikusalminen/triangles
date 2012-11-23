project(glproto C)
cmake_minimum_required(VERSION 2.8)

if (CMAKE_COMPILER_IS_GNUCC)
    add_definitions(-std=gnu99)
    add_definitions(-W -Wall -Wextra)

    link_libraries(m)
endif()

add_subdirectory(ext)

include_directories(ext/glwt/include)
include_directories(ext/threedee-simd/include)

include_directories(${CMAKE_BINARY_DIR}/ext/glwt/ext/glxw/include)

add_subdirectory(src)
