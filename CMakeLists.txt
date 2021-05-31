cmake_minimum_required(VERSION 3.10)
project(tin)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall)

include_directories(utils)
add_subdirectory(utils)

add_subdirectory(picoquic_impl)
add_subdirectory(msquic_impl)
add_subdirectory(tcp)
add_subdirectory(main_app)

enable_testing()
add_subdirectory(tests)