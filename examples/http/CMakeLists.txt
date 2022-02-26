set(http_SRCS
    HTTPRequest.cc
    HTTPResponse.cc
    HTTPCodec.cc
)

add_library(http ${http_SRCS})
target_link_libraries(http slite_net)

install(TARGETS http DESTINATION lib)
set(HEADERS
    HTTPRequest.h
    HTTPResponse.h
    HTTPCodec.h
)
install(FILES ${HEADERS} DESTINATION include/http)

add_executable(httpserver HTTPServer.cc)
target_link_libraries(httpserver http)
