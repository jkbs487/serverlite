add_executable(simple simple.cc)
target_link_libraries(simple net)
target_link_libraries(simple pthread)

add_executable(maxconnection maxconnection.cc)
target_link_libraries(maxconnection net)
target_link_libraries(maxconnection pthread)

add_executable(file filetransfer.cc)
target_link_libraries(file net)
target_link_libraries(file pthread)