set(INCLUDES
${HalideInclude}
${TIRAMISU_ROOT}/include/
${TIRAMISU_ROOT}/3rdParty/isl/include
/home/sk10691/.conda/envs/new-main/include/
${PROJECT_SOURCE_DIR}/include
)

set(HEADER_FILES
${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/utils.h
)

# add_subdirectory(functions)


add_library(HermesII SHARED utils.cpp)
target_include_directories(HermesII PUBLIC ${INCLUDES})
target_link_directories(HermesII PUBLIC ${TIRAMISU_ROOT}/build ${HalideLib} ${TIRAMISU_ROOT}/3rdParty/isl/build/lib)
target_link_libraries(HermesII tiramisu tiramisu_auto_scheduler Halide isl sqlite3)
set_target_properties(HermesII PROPERTIES PUBLIC_HEADER "${HEADER_FILES}")
install(TARGETS HermesII LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include/${PROJECT_NAME})



# add_executable(main main.cpp)
# target_link_directories(main PUBLIC ${TIRAMISU_ROOT}/build ${HalideLib} ${TIRAMISU_ROOT}/3rdParty/isl/build/lib)
# target_include_directories(main PUBLIC ${INCLUDES})
# target_link_libraries(main HermesII tiramisu tiramisu_auto_scheduler Halide isl sqlite3 ZLIB::ZLIB)
