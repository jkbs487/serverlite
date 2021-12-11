add_executable(simple simple.cc)
target_link_libraries(simple net)

add_executable(maxconnection maxconnection.cc)
target_link_libraries(maxconnection net)

add_executable(file filetransfer.cc)
target_link_libraries(file net)

add_executable(oneshot oneshot.cc)
target_link_libraries(oneshot net)