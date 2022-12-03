add_executable(eventloop_test EventLoopTest.cc)
target_link_libraries(eventloop_test slite_net)

add_executable(eventloopthread_test EventLoopThreadTest.cc)
target_link_libraries(eventloopthread_test slite_net)

add_executable(eventloopthreadpool_test EventLoopThreadPoolTest.cc)
target_link_libraries(eventloopthreadpool_test slite_net)

add_executable(logger_test LoggerTest.cc)
target_link_libraries(logger_test slite_net)

add_executable(logging_test LoggingTest.cc)
target_link_libraries(logging_test slite_net)

add_executable(asynclogger_test AsyncLoggerTest.cc)
target_link_libraries(asynclogger_test slite_net)

add_executable(reactor_test05 ReactorTest05.cc)
target_link_libraries(reactor_test05 slite_net)

add_executable(tcpclient_test01 TCPClientTest01.cc)
target_link_libraries(tcpclient_test01 slite_net)

add_executable(tcpclient_test02 TCPClientTest02.cc)
target_link_libraries(tcpclient_test02 slite_net)

add_executable(tcpclient_test03 TCPClientTest03.cc)
target_link_libraries(tcpclient_test03 slite_net)

add_executable(threadpool_test ThreadPoolTest.cc)
target_link_libraries(threadpool_test slite_net)

if(PROTOBUF_FOUND)
    add_custom_command(OUTPUT query.pb.cc query.pb.h
    COMMAND protoc
    ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/query.proto -I${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS query.proto
    VERBATIM )

    set_source_files_properties(query.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")
    include_directories(${PROJECT_BINARY_DIR})

    add_library(query_proto query.pb.cc)
    target_link_libraries(query_proto protobuf pthread)

    add_executable(protobuf_codec_test ProtobufCodecTest.cc)
    set_target_properties(protobuf_codec_test PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
    target_link_libraries(protobuf_codec_test slite_protobuf_codec query_proto)
    
    add_executable(protobuf_dispatcher_test ProtobufDispatcherTest.cc)
    set_target_properties(protobuf_dispatcher_test PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
    target_link_libraries(protobuf_dispatcher_test query_proto)

    add_custom_target(protobuf_codec_all
    DEPENDS
          protobuf_codec_test
          #protobuf_dispatcher_lite_test
          protobuf_dispatcher_test
          #protobuf_server
          #protobuf_client
          )
endif()