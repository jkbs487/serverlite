add_executable(echoserver_test EchoServerTest.cc)
target_link_libraries(echoserver_test net)

add_executable(eventloop_test EventLoopTest.cc)
target_link_libraries(eventloop_test net)

add_executable(eventloopthread_test EventLoopThreadTest.cc)
target_link_libraries(eventloopthread_test net)

add_executable(eventLoopthreadpool_est EventLoopThreadPoolTest.cc)
target_link_libraries(eventLoopthreadpool_est net)

add_executable(logger_test LoggerTest.cc)
target_link_libraries(logger_test net)

add_executable(asynclogger_test AsyncLoggerTest.cc)
target_link_libraries(asynclogger_test net)

add_executable(reactor_test05 ReactorTest05.cc)
target_link_libraries(reactor_test05 net)

add_executable(tcpclient_test01 TCPClientTest01.cc)
target_link_libraries(tcpclient_test01 net)

add_executable(tcpclient_test02 TCPClientTest02.cc)
target_link_libraries(tcpclient_test02 net)

add_executable(tcpclient_test03 TCPClientTest03.cc)
target_link_libraries(tcpclient_test03 net)
