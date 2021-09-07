include(CheckFunctionExists)

set(net_SRCS
    Channel.cc
    Acceptor.cc
    EventLoop.cc
    TCPServer.cc
    TCPConnection.cc
    EventLoopThread.cc
    EventLoopThreadPool.cc
)

add_library(net ${net_SRCS})
target_link_libraries(net pthread)
install(TARGETS net DESTINATION lib)

set(HEADERS 
    Channel.h
    Acceptor.h
    EventLoop.h
    TCPServer.h
    TCPConnection.h
    EventLoopThread.h
    EventLoopThreadPool.h
)
install(FILES ${HEADERS} DESTINATION include)

add_subdirectory(test)