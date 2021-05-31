add_subdirectory(msquic)

add_executable(ms_server server.c)
target_link_libraries(ms_server inc warnings msquic platform inc utils)

add_executable(ms_client client.c)
target_link_libraries(ms_client inc warnings msquic platform inc utils)