set(net_SRCS
Channel.h
TCPConnection.h
TCPServer.h
)

add_executable(simple ${net_SRCS} simple.cc)