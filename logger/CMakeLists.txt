# TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
# Utworzono: 03.06.2021
# Autor: Piotr Sawicki

add_executable(logger_test test.cpp)
target_link_libraries(logger_test gtest_main rt)

add_test(NAME logger_test COMMAND logger_test)