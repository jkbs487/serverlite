add_executable(simple_server simple_server.cc)
target_link_libraries(simple_server net)

add_executable(simple_client simple_client.cc)
target_link_libraries(simple_client net)

add_executable(max_connection max_connection.cc)
target_link_libraries(max_connection net)

add_executable(file_server file_server.cc)
target_link_libraries(file_server net)

add_executable(oneshot oneshot.cc)
target_link_libraries(oneshot net)

add_executable(pingpong_bench pingpong_bench.cc)
target_link_libraries(pingpong_bench net)

add_executable(subpub subpub.cc)
target_link_libraries(subpub net)

add_subdirectory(http)

if(PROTOBUF_FOUND)
    add_subdirectory(protobuf)
    add_subdirectory(loginserver)
endif()
