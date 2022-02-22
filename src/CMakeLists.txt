include(CheckFunctionExists)

set(net_SRCS
    Timer.cc
    Channel.cc
    Acceptor.cc
    Connector.cc
    EventLoop.cc
    TCPClient.cc
    TCPServer.cc
    AsyncLogger.cc
    TCPConnection.cc
    EventLoopThread.cc
    EventLoopThreadPool.cc
    Logger.cc
    ThreadPool.cc
)

add_library(net ${net_SRCS})
target_link_libraries(net pthread)
install(TARGETS net DESTINATION lib)

set(HEADERS 
    Timer.h
    Logger.h
    Channel.h
    Acceptor.h
    Connector.h
    EventLoop.h
    TCPClient.h
    TCPServer.h
    AsyncLogger.h
    TCPConnection.h
    EventLoopThread.h
    EventLoopThreadPool.h
    Logger.h
    ThreadPool.h
)
install(FILES ${HEADERS} DESTINATION include)

add_subdirectory(test)

if(PROTOBUF_FOUND)
endif()