set(dbproxyserver_SRCS
  EncDec.cc
  DBProxyServer.cc
  models/DBPool.cc
  models/DepartmentModel.cc
  models/UserModel.cc
)

add_executable(dbproxyserver ${dbproxyserver_SRCS})
target_link_libraries(dbproxyserver slite_net protobuf_codec im_proto mysqlclient ssl crypto z)