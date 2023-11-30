cmake_minimum_required(VERSION 3.22)
project(HermesII)

# set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_STANDARD 17)  # or newer
# set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -g -fno-rtti -lpthread")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS>  <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

# old Halide
# set(TIRAMISU_ROOT /scratch/sk10691/workspace/tiramisu)
# set(HalideInclude ${TIRAMISU_ROOT}/3rdParty/Halide/include)
# set(HalideLib ${TIRAMISU_ROOT}/3rdParty/Halide/lib)
set(ENV_INCLUDES /home/sk10691/.conda/envs/new-main/include)

find_package(ZLIB REQUIRED)

# # new Halide and Tiramisu
set(TIRAMISU_ROOT /home/sk10691/tiramisu_new)
set(HalideInclude ${TIRAMISU_ROOT}/3rdParty/Halide/install/include)
set(HalideLib ${TIRAMISU_ROOT}/3rdParty/Halide/install/lib64)

add_subdirectory(src)

option(BUILD_EXAMPLES "Build examples or not" OFF)
option(BUILD_TESTS "Build tests or not" OFF)


# if(BUILD_TESTS)
#     message(STATUS "Building tests...")
#     add_subdirectory(tests)
# endif()

if(BUILD_EXAMPLES)
    message(STATUS "Building examples...")
    add_subdirectory(examples)
endif()




