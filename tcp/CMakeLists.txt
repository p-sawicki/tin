set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tcp/cmake")
find_package(OpenSSL REQUIRED)

add_executable(tcp_server server.c)
target_link_libraries(tcp_server ${OPENSSL_LIBRARIES})

add_executable(tcp_client client.c)
target_link_libraries(tcp_client ${OPENSSL_LIBRARIES})