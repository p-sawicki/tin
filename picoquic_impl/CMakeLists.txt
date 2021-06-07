execute_process(
  COMMAND ${CMAKE_COMMAND} 
  -DDIR=${CMAKE_CURRENT_SOURCE_DIR}/picotls
  -DC_COMPILER=${CMAKE_C_COMPILER}
  -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
  -DGENERATOR=${CMAKE_GENERATOR}
  -P ${CMAKE_CURRENT_SOURCE_DIR}/build_picotls.cmake
)

add_subdirectory(picoquic)
include_directories(picoquic/picoquic)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/picoquic/cmake")
find_package(PTLS REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

set(pico_common 
  picoquic-core
  ${PTLS_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  utils
)

add_executable(pico_server server.c)
target_link_libraries(pico_server ${pico_common})

add_executable(pico_client client.c)
target_link_libraries(pico_client ${pico_common})