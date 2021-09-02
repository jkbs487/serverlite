add_executable(simple simple.cc)
target_link_libraries(simple net)
target_link_libraries(simple pthread)

add_executable(file filetransfer.cc)
target_link_libraries(file net)
target_link_libraries(file pthread)