set(PROTOS
  IM.BaseDefine.pb.cc IM.BaseDefine.pb.h
  IM.Other.pb.cc IM.Other.pb.h
  IM.Server.pb.cc IM.Server.pb.h
  IM.Login.pb.cc IM.Login.pb.h
)

add_custom_command(OUTPUT ${PROTOS}
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/IM.BaseDefine.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/IM.Other.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/IM.Server.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/IM.Login.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS IM.BaseDefine.proto IM.Other.proto IM.Server.proto IM.Login.proto
  VERBATIM )

set_source_files_properties(IM.BaseDefine.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
set_source_files_properties(IM.Other.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
set_source_files_properties(IM.Server.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
set_source_files_properties(IM.Login.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
include_directories(${PROJECT_BINARY_DIR})

add_library(im_basedefine_proto IM.BaseDefine.pb.cc)
target_link_libraries(im_basedefine_proto protobuf pthread)

add_library(im_other_proto IM.Other.pb.cc)
target_link_libraries(im_other_proto protobuf pthread)

add_library(im_server_proto IM.Server.pb.cc)
target_link_libraries(im_server_proto im_basedefine_proto protobuf pthread)

add_library(im_login_proto IM.Login.pb.cc)
target_link_libraries(im_login_proto im_basedefine_proto protobuf pthread)

add_executable(loginserver LoginServer.cc)
target_link_libraries(loginserver net http protobuf_codec 
                      im_basedefine_proto im_other_proto im_server_proto im_login_proto)

add_executable(msgserver MsgServer.cc)
target_link_libraries(msgserver net protobuf_codec 
                      im_basedefine_proto im_other_proto im_server_proto im_login_proto)