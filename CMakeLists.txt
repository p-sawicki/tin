# TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
# Utworzono: 05.05.2021
# Autor: Piotr Sawicki

cmake_minimum_required(VERSION 3.10)
project(tin)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall)

include_directories(utils
  msquic_impl/msquic/submodules/googletest/googletest/include
)
add_subdirectory(utils)

add_subdirectory(picoquic_impl)
add_subdirectory(msquic_impl)
add_subdirectory(tcp)
add_subdirectory(mvfst_impl)
add_subdirectory(main_app)

enable_testing()
add_subdirectory(tests)
add_subdirectory(logger)