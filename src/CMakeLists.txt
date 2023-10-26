set(INCLUDES
# ${TIRAMISU_ROOT}/3rdParty/Halide/install/include
${HalideInclude}
${TIRAMISU_ROOT}/include/
${TIRAMISU_ROOT}/3rdParty/isl/include
/home/sk10691/.conda/envs/new-main/include/
${CMAKE_CURRENT_SOURCE_DIR}/../include
)



# Protobuf
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

# Protobuf-compiler
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

# gRPC
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# Proto file
get_filename_component(hw_proto "tiramisuserver.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/tiramisuserver.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/tiramisuserver.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/tiramisuserver.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/tiramisuserver.grpc.pb.h")
add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Targets (client|server)
# foreach(_target
#   client server)
#   add_executable(${_target} "${_target}.cc"
#     ${hw_proto_srcs}
#     ${hw_grpc_srcs})
#   target_link_libraries(${_target}
#     ${_REFLECTION}
#     ${_GRPC_GRPCPP}
#     ${_PROTOBUF_LIBPROTOBUF})
# endforeach()

# file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${HermesII_SOURCE_DIR}/include/hermes_ii/*.hpp")
# # Make an automatic library - will be static or dynamic based on user setting
# add_library(hermes_ii functions.cpp utils.cpp ${HEADER_LIST})

# target_include_directories(hermes_ii PUBLIC ../include)

# Get all cpps in functions directory

add_subdirectory(functions)
add_subdirectory(switches)

add_executable(main server.cc switchMap.cpp utils.cpp ${hw_proto_srcs} ${hw_grpc_srcs})


target_link_directories(main PUBLIC ${TIRAMISU_ROOT}/build ${HalideLib} ${TIRAMISU_ROOT}/3rdParty/isl/build/lib)

target_include_directories(main PUBLIC ${INCLUDES})
# target_link_libraries(main tiramisu tiramisu_auto_scheduler Halide isl autoSchedulerInfrastructure ${PROJECT_NAME})
target_link_libraries(main functions000 functions001 functions002 functions003 functions004 functions005 functions006 functions007 functions008 functions009 functions010 functions011 functions012 functions013 functions014 functions015 functions016 functions017 functions018 functions019 functions020 functions021 functions022 functions023 functions024 functions025 functions026 functions027 functions028 functions029 functions030 functions031 functions032 functions033 functions034 functions035 functions036 functions037 functions038 functions039 functions040 functions041 functions042 functions043 functions044 functions045 functions046 functions047 functions048 functions049 functions050 functions051 functions052 functions053 functions054 functions055 functions056 functions057 functions058 functions059 functions060 functions061 functions062 functions063 functions064 functions065 functions066 functions067 functions068 functions069 functions070 functions071 functions072 functions073 functions074 functions075 functions076 functions077 functions078 functions079 functions080 functions081 functions082 functions083 functions084 functions085 functions086 functions087 functions088 functions089 functions090 functions091 functions092 functions093 functions094 functions095 functions096 functions097 functions098 functions099 functions100 functions101 functions102 functions103 functions104 functions105 functions106 functions107 functions108 functions109 functions110 functions111 functions112 functions113 functions114 functions115 functions116 functions117 functions118 functions119 functions120 functions121 functions122 functions123 functions124 functions125 functions126 functions127 functions128 functions129 functions130 functions131 functions132 functions133 functions134 functions135 functions136 functions137 functions138 functions139 functions140 functions141 functions142 functions143 functions144 functions145 functions146 functions147 functions148 functions149 functions150 functions151 functions152 functions153 functions154 functions155 functions156 functions157 functions158 functions159 functions160 functions161 functions162 functions163 functions164 functions165 functions166 functions167 functions168 functions169 functions170 functions171 functions172 functions173 functions174 functions175 functions176 functions177 functions178 functions179 functions180 functions181 functions182 functions183 functions184 functions185 functions186 functions187 functions188 functions189 functions190 functions191 functions192 functions193 functions194 functions195 functions196 functions197 functions198 functions199 functions200 functions201 functions202 functions203 functions204 functions205 functions206 functions207 functions208 functions209 functions210 functions211 functions212 functions213 functions214 functions215 functions216 functions217 functions218 functions219 functions220 functions221 functions222 functions223 functions224 functions225 functions226 functions227 functions228 functions229 functions230 functions231 functions232 functions233 functions234 functions235 functions236 functions237 functions238 functions239 functions240 functions241 functions242 functions243 functions244 functions245 functions246 functions247 functions248 functions249 functions250 functions251 functions252 functions253 functions254 functions255 functions256 functions257 functions258 functions259 functions260 functions261 functions262 functions263 functions264 functions265 functions266 functions267 functions268 functions269 functions270 functions271 functions272 functions273 functions274 functions275 functions276 functions277 functions278 functions279 functions280 functions281 functions282 functions283 functions284 functions285 functions286 functions287 functions288 functions289 functions290 functions291 functions292 functions293 functions294 functions295 functions296 functions297 switch0 switch1 switch10 switch11 switch12 switch13 switch14 switch15 switch16 switch17 switch18 switch19 switch2 switch20 switch21 switch22 switch23 switch24 switch25 switch26 switch27 switch28 switch29 switch3 switch30 switch31 switch32 switch33 switch34 switch35 switch36 switch37 switch38 switch39 switch4 switch40 switch41 switch42 switch43 switch44 switch45 switch46 switch47 switch48 switch49 switch5 switch50 switch51 switch52 switch53 switch54 switch55 switch56 switch57 switch58 switch59 switch6 switch60 switch61 switch62 switch63 switch64 switch65 switch66 switch67 switch68 switch69 switch7 switch70 switch71 switch72 switch73 switch74 switch75 switch76 switch77 switch78 switch79 switch8 switch80 switch81 switch82 switch83 switch84 switch85 switch86 switch87 switch88 switch89 switch9 switch90 switch91 switch92 switch93 switch94 switch95 switch96 switch97 switch98 switch99  tiramisu tiramisu_auto_scheduler Halide isl sqlite3 ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})

# target_link_libraries(main tiramisu tiramisu_auto_scheduler Halide isl sqlite3 ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})
