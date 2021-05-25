add_subdirectory(picotls)

cmake_host_system_information(RESULT core_count QUERY NUMBER_OF_LOGICAL_CORES)
execute_process(
  COMMAND cmake --build . -- -j${core_count}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/picotls
  RESULT_VARIABLE result
)

add_subdirectory(picoquic)
include_directories(picoquic/loglib picoquic/picoquic)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/picoquic/cmake")
find_package(PTLS REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_library(pico_common common.c)
target_link_libraries(pico_common 
  picoquic-log
  picoquic-core
  ${PTLS_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(pico_server server.c)
target_link_libraries(pico_server pico_common)

add_executable(pico_client client.c)
target_link_libraries(pico_client pico_common)