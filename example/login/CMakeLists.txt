set(SERVER_LIMIT_LIST serverLimit.cc ../user.pb.cc)
set(CLIENT_CIRCUIT_BREAK_LIST client_circuit_break.cc ../user.pb.cc)
set(CLIENT1_LIST client1.cc ../user.pb.cc)
set(CLIENT2_LIST client2.cc ../user.pb.cc)
set(CLIENT_ASYNC_LIST client_async.cc ../user.pb.cc)
set(CLIENT_FAILOVER client_failover.cc ../user.pb.cc)
set(SERVER_LIST server.cc ../user.pb.cc)
set(SERVER_HTTP_LIST serverHttp.cc ../user.pb.cc)

add_executable(serverLimit ${SERVER_LIMIT_LIST})
add_executable(client_circuit_break ${CLIENT_CIRCUIT_BREAK_LIST})
add_executable(client1 ${CLIENT1_LIST})
add_executable(client2 ${CLIENT2_LIST})
add_executable(client_async ${CLIENT_ASYNC_LIST})
add_executable(client_failover ${CLIENT_FAILOVER})
add_executable(server ${SERVER_LIST})
add_executable(serverHttp ${SERVER_HTTP_LIST})

target_link_libraries(serverLimit PRIVATE mprpc protobuf)
target_link_libraries(serverLimit PRIVATE "-L/usr/local/lib/ -lprotobuf")
target_link_libraries(client_circuit_break PRIVATE mprpc protobuf)
target_link_libraries(client_circuit_break PRIVATE "-L/usr/local/lib/ -lprotobuf")
target_link_libraries(client1 PRIVATE mprpc protobuf)
target_link_libraries(client1 PRIVATE "-L/usr/local/lib/ -lprotobuf")
target_link_libraries(client2 PRIVATE mprpc protobuf)
target_link_libraries(client2 PRIVATE "-L/usr/local/lib/ -lprotobuf")
target_link_libraries(client_async PRIVATE mprpc protobuf)
target_link_libraries(client_async PRIVATE "-L/usr/local/lib/ -lprotobuf")
target_link_libraries(client_failover PRIVATE mprpc protobuf)
target_link_libraries(client_failover PRIVATE "-L/usr/local/lib/ -lprotobuf")
target_link_libraries(server PRIVATE mprpc protobuf)
target_link_libraries(server PRIVATE "-L/usr/local/lib/ -lprotobuf")
target_link_libraries(serverHttp PRIVATE mprpc protobuf)
target_link_libraries(serverHttp PRIVATE "-L/usr/local/lib/ -lprotobuf")