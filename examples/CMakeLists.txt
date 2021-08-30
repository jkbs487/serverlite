add_executable(simple simple.cc)
target_link_libraries(simple net)

add_executable(file filetransfer.cc)
target_link_libraries(file net)