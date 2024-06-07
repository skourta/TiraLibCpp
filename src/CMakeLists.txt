set(INCLUDES
${HalideInclude}
${TIRAMISU_ROOT}/include/
${TIRAMISU_ROOT}/3rdParty/isl/include
/home/sk10691/.conda/envs/new-main/include/
${PROJECT_SOURCE_DIR}/include
)

option(USE_SQLITE "Use SQLITE" OFF)
if (USE_SQLITE)
  MESSAGE(STATUS "USE_SQLITE")
  add_definitions(-DUSE_SQLITE)
endif()

set(HEADER_FILES
${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/utils.h
${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/actions.h
)

if (USE_SQLITE)
    list(APPEND HEADER_FILES ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/dbhelpers.h)
endif()

set(SOURCES utils.cc actions.cc)
if (USE_SQLITE)
    list(APPEND SOURCES dbhelpers.cc)
endif()
add_library(TiraLibCPP SHARED ${SOURCES})

target_include_directories(TiraLibCPP PUBLIC ${INCLUDES})
target_link_directories(TiraLibCPP PUBLIC ${TIRAMISU_ROOT}/build ${HalideLib} ${TIRAMISU_ROOT}/3rdParty/isl/build/lib)

set(LIBRARIES tiramisu tiramisu_auto_scheduler Halide isl)
if (USE_SQLITE)
    list(APPEND LIBRARIES sqlite3)
endif()
target_link_libraries(TiraLibCPP ${LIBRARIES})
set_target_properties(TiraLibCPP PROPERTIES PUBLIC_HEADER "${HEADER_FILES}")
install(TARGETS TiraLibCPP LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include/${PROJECT_NAME})
