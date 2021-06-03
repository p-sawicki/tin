cmake_host_system_information(RESULT core_count QUERY NUMBER_OF_LOGICAL_CORES)
execute_process(
  COMMAND ${CMAKE_COMMAND} --configure . -DCMAKE_C_COMPILER=${C_COMPILER}
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