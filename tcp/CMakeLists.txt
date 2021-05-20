set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tcp/cmake")

add_executable(tcp_server server.c)
target_link_libraries(tcp_server)

add_executable(tcp_client client.c)
target_link_libraries(tcp_client)