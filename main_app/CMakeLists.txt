# TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
# Utworzono: 12.05.2021
# Autor: Piotr Sawicki

add_executable(run_scenarios 
  run_scenarios.cpp
  Logger.cpp
  utils.cpp
)
target_link_libraries(run_scenarios rt)