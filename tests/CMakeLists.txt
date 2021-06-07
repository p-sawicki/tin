# TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
# Utworzono: 29.05.2021
# Autor: Piotr Sawicki

function(add_client_tests IMPL PORT REG1 REG2 REG3 REG4)
  add_test(
    NAME ${IMPL}_1
    COMMAND ${IMPL}_client 127.0.0.1 ${PORT} 1 --no-delay
  )
  set_tests_properties(${IMPL}_1 PROPERTIES PASS_REGULAR_EXPRESSION ${REG1})

  add_test(
    NAME ${IMPL}_2
    COMMAND ${IMPL}_client 127.0.0.1 ${PORT} 2 --no-delay
  )
  set_tests_properties(${IMPL}_2 PROPERTIES PASS_REGULAR_EXPRESSION ${REG2})

  add_test(
    NAME ${IMPL}_3
    COMMAND ${IMPL}_client 127.0.0.1 ${PORT} 3 --no-delay
  )
  set_tests_properties(${IMPL}_3 PROPERTIES PASS_REGULAR_EXPRESSION ${REG3})

  add_test(
    NAME ${IMPL}_4
    COMMAND ${IMPL}_client 127.0.0.1 ${PORT} 4 --no-delay
  )
  set_tests_properties(${IMPL}_4 PROPERTIES PASS_REGULAR_EXPRESSION ${REG4})

  add_test(
    NAME ${IMPL}_unsupported
    COMMAND ${IMPL}_client 127.0.0.1 ${PORT} 5 --no-delay
  )
  set_tests_properties(${IMPL}_unsupported PROPERTIES WILL_FAIL TRUE)
endfunction(add_client_tests)

add_client_tests(
  pico 
  1111 
  ".* received 100 bytes" 
  ".* received 1073741824 bytes"
  ".*received 100 bytes"
  ".*received 10485760 bytes"
)

add_client_tests(
  ms
  1112
  ".* bytes received: 100"
  ".* bytes received: 1073741824"
  ".*Total bytes received: 1000"
  ".*Total bytes received: 104857600"
)

add_client_tests(
  tcp
  1113
  ".*Received: 100 bytes"
  ".*Received: 1073741824 bytes"
  ".*Received: 100 bytes"
  ".*Received: 10485760 bytes"
)