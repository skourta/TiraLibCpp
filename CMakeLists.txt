cmake_minimum_required(VERSION 3.22)
project(TiraLibCPP)

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_STANDARD 17) # or newer
set(CMAKE_CXX_EXTENSIONS NO)

# Flags for Tiramisu
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -g -fno-rtti -lpthread")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS>  <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

find_package(ZLIB REQUIRED)

add_subdirectory(src)

option(BUILD_EXAMPLES "Build examples or not" OFF)
option(BUILD_TESTS "Build tests or not" OFF)

if(BUILD_TESTS)
    message(STATUS "Building tests...")
    add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    message(STATUS "Building examples...")
    add_subdirectory(examples)
endif()

message(STATUS "Tiramisu install is ${TIRAMISU_INSTALL}")