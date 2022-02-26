add_executable(loginserver LoginServer.cc)
target_link_libraries(loginserver slite_net http protobuf_codec z im_proto)