include(CheckFunctionExists)

set(net_SRCS
    Timer.cc
    Channel.cc
    TCPHandle.cc
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
    Logging.cc
    ThreadPool.cc
)

add_library(slite_net ${net_SRCS})
target_link_libraries(slite_net pthread)
install(TARGETS slite_net DESTINATION lib)

set(HEADERS 
    Timer.h
    Logger.h
    Channel.h
    Acceptor.h
    TCPHandle.h
    Connector.h
    EventLoop.h
    TCPClient.h
    TCPServer.h
    AsyncLogger.h
    TCPConnection.h
    EventLoopThread.h
    EventLoopThreadPool.h
    Logger.h
    Logging.h
    ThreadPool.h
)
install(FILES ${HEADERS} DESTINATION include/slite)

add_subdirectory(test)
add_subdirectory(http)

if(PROTOBUF_FOUND)
    add_subdirectory(protobuf)
    #add_subdirectory(protorpc)
endif()
