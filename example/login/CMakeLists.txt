set(CLIENT_LIST client.cc ../user.pb.cc)
set(CLIENT_MULTI_LIST client_multi_thread.cc ../user.pb.cc)
set(SERVER_LIST server.cc ../user.pb.cc)

add_executable(client ${CLIENT_LIST})
add_executable(client_multi_thread ${CLIENT_MULTI_LIST})
add_executable(server ${SERVER_LIST})

target_link_libraries(client PRIVATE mprpc protobuf)
target_link_libraries(client PRIVATE "-L/usr/local/lib/ -lprotobuf")
target_link_libraries(client_multi_thread PRIVATE mprpc protobuf)
target_link_libraries(client_multi_thread PRIVATE "-L/usr/local/lib/ -lprotobuf")
target_link_libraries(server PRIVATE mprpc protobuf)
target_link_libraries(server PRIVATE "-L/usr/local/lib/ -lprotobuf")