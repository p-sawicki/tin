# TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
# Utworzono: 29.05.2021
# Autor: Adam Solawa

find_package(OpenSSL REQUIRED)

add_library(tcp_common common.c)
target_link_libraries(tcp_common 
  ${OPENSSL_LIBRARIES}
  utils
)

add_executable(tcp_server server.c)
target_link_libraries(tcp_server tcp_common pthread)

add_executable(tcp_client client.c)
target_link_libraries(tcp_client tcp_common)