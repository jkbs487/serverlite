#add_executable(bench_client bench_client.cc)

add_executable(simple_server simple_server.cc)
target_link_libraries(simple_server net)

add_executable(simple_client simple_client.cc)
target_link_libraries(simple_client net)

add_executable(max_connection max_connection.cc)
target_link_libraries(max_connection net)

add_executable(file filetransfer.cc)
target_link_libraries(file net)

add_executable(oneshot oneshot.cc)
target_link_libraries(oneshot net)

add_executable(http http.cc)
target_link_libraries(http net)