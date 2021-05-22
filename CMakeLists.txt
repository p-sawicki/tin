cmake_minimum_required(VERSION 3.10)
project(tin)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(picoquic_impl)
add_subdirectory(quic/msquic)
add_subdirectory(msquic_impl)
add_subdirectory(main_app)