set(PROTOS
  IM.BaseDefine.pb.cc IM.BaseDefine.pb.h
  IM.Other.pb.cc IM.Other.pb.h
  IM.Server.pb.cc IM.Server.pb.h
  IM.Login.pb.cc IM.Login.pb.h
  IM.Buddy.pb.cc IM.Buddy.pb.h
  IM.Group.pb.cc IM.Group.pb.h
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
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/IM.Buddy.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND protoc
  ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/IM.Group.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS IM.BaseDefine.proto IM.Other.proto IM.Server.proto IM.Login.proto IM.Buddy.proto IM.Group.proto
  VERBATIM )

set_source_files_properties(IM.BaseDefine.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
set_source_files_properties(IM.Other.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
set_source_files_properties(IM.Server.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
set_source_files_properties(IM.Login.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
set_source_files_properties(IM.Buddy.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
set_source_files_properties(IM.Group.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
include_directories(${PROJECT_BINARY_DIR})

set(proto_SRCS
  IM.BaseDefine.pb.cc 
  IM.Other.pb.cc 
  IM.Server.pb.cc 
  IM.Login.pb.cc 
  IM.Buddy.pb.cc 
  IM.Group.pb.cc
)

add_library(im_proto ${proto_SRCS})
target_link_libraries(im_proto protobuf pthread)