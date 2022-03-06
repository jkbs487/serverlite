set(msgserver_SRCS
    MsgServer.cc
    ImUser.cc
    ../base/protobuf_codec.cc
)

add_executable(msgserver ${msgserver_SRCS})
target_link_libraries(msgserver slite_net protobuf_codec z im_proto)