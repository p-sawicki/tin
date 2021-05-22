set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../quic/msquic/cmake")

add_executable(ms_server server.c)
target_link_libraries(ms_server inc warnings msquic platform inc)

add_executable(ms_client client.c)
target_link_libraries(ms_client inc warnings msquic platform inc)
