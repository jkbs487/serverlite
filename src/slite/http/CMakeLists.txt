set(http_SRCS
    HTTPRequest.cc
    HTTPResponse.cc
    HTTPCodec.cc
    HTTPServer.cc
)

add_library(slite_http ${http_SRCS})
target_link_libraries(slite_http slite_net)

install(TARGETS slite_http DESTINATION lib)
set(HEADERS
    HTTPRequest.h
    HTTPResponse.h
    HTTPCodec.h
    HTTPServer.h
)
install(FILES ${HEADERS} DESTINATION include/slite/http)
