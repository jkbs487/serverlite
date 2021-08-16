set(net_SRCS
    EventLoop.cc
    Channel.cc
    TCPConnection.cc
    TCPServer.cc
)

add_library(net ${net_SRCS})

add_executable(simple simple.cc)
target_link_libraries(simple net)

add_executable(file filetransfer.cc)
target_link_libraries(file net)
