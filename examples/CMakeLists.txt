add_executable(simple_server simple_server.cc)
target_link_libraries(simple_server slite_net)

add_executable(simple_client simple_client.cc)
target_link_libraries(simple_client slite_net)

add_executable(max_connection max_connection.cc)
target_link_libraries(max_connection slite_net)

add_executable(file_server file_server.cc)
target_link_libraries(file_server slite_net)

add_executable(oneshot oneshot.cc)
target_link_libraries(oneshot slite_net)

add_executable(pingpong_bench pingpong_bench.cc)
target_link_libraries(pingpong_bench slite_net)

add_executable(subpub subpub.cc)
target_link_libraries(subpub slite_net)

add_executable(sing sing.cc)
target_link_libraries(sing slite_net)

add_subdirectory(http)
