add_executable(EchoServerTest EchoServerTest.cc)
target_link_libraries(EchoServerTest net)

add_executable(EventLoopTest EventLoopTest.cc)
target_link_libraries(EventLoopTest net)

add_executable(EventLoopThreadTest EventLoopThreadTest.cc)
target_link_libraries(EventLoopThreadTest net)

add_executable(EventLoopThreadPoolTest EventLoopThreadPoolTest.cc)
target_link_libraries(EventLoopThreadPoolTest net)

add_executable(LoggerTest LoggerTest.cc)
target_link_libraries(LoggerTest net)

add_executable(ReactorTest05 ReactorTest05.cc)
target_link_libraries(ReactorTest05 net)

add_executable(TCPClientTest01 TCPClientTest01.cc)
target_link_libraries(TCPClientTest01 net)

add_executable(TCPClientTest02 TCPClientTest02.cc)
target_link_libraries(TCPClientTest02 net)

add_executable(TCPClientTest03 TCPClientTest03.cc)
target_link_libraries(TCPClientTest03 net)