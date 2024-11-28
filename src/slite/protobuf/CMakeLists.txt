add_library(slite_protobuf_codec ProtobufCodec.cc ProtobufCodecLite.cc)
target_link_libraries(slite_protobuf_codec protobuf slite_net z)

install(TARGETS slite_protobuf_codec DESTINATION lib)
set(HEADERS
  ProtobufDispatcher.h
  ProtobufCodec.h
  ProtobufCodecLite.h
)
install(FILES ${HEADERS} DESTINATION include/slite/protobuf)