# TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
# Utworzono: 03.06.2021
# Autor: Piotr Sawicki

cmake_host_system_information(RESULT core_count QUERY NUMBER_OF_LOGICAL_CORES)
execute_process(
  COMMAND ${CMAKE_COMMAND} -B . -DCMAKE_C_COMPILER=${C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -G${GENERATOR}
  WORKING_DIRECTORY ${DIR}
  RESULT_VARIABLE result
)

if (result)
  message(ERROR " Could not configure picotls, code: ${result}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build . -- -j${core_count}
  WORKING_DIRECTORY ${DIR}
  RESULT_VARIABLE result
)

if (result)
  message(ERROR " Could not build picotls, code: ${result}")
endif()