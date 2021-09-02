include(CheckFunctionExists)

set(net_SRCS
    EventLoop.cc
    Channel.cc
    TCPConnection.cc
    TCPServer.cc
    EventLoopThread.cc
    EventLoopThreadPool.cc
)

add_library(net ${net_SRCS})
install(TARGETS net DESTINATION lib)

set(HEADERS 
    EventLoop.h
    Channel.h
    TCPConnection.h
    TCPServer.h
    EventLoopThread.h
    EventLoopThreadPool.h
)
install(FILES ${HEADERS} DESTINATION include)

add_subdirectory(test)