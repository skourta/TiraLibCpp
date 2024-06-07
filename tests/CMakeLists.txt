
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(INCLUDES
${HalideInclude}
message("Tiramisu root is ${TIRAMISU_ROOT}")
${TIRAMISU_ROOT}/include/
${TIRAMISU_ROOT}/3rdParty/isl/include
/home/sk10691/.conda/envs/new-main/include/
${PROJECT_SOURCE_DIR}/include
)

enable_testing()

add_executable(
  actions_test
  actions_test.cc
)

target_link_directories(actions_test PUBLIC ${TIRAMISU_ROOT}/build ${HalideLib} ${TIRAMISU_ROOT}/3rdParty/isl/build/lib)

target_link_libraries(
  actions_test
  GTest::gtest_main
  tiramisu
  tiramisu_auto_scheduler 
  Halide 
  isl
  ZLIB::ZLIB
  TiraLibCPP
)

target_include_directories(actions_test PUBLIC ${INCLUDES})


include(GoogleTest)
gtest_discover_tests(actions_test)