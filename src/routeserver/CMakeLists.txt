set(routeserver_SRCS
    RouteServer.cc
    UserManager.cc
)

add_executable(routeserver ${routeserver_SRCS})
target_link_libraries(routeserver slite_net protobuf_codec z im_proto)
