# TIN - Porównanie wybranych implementacji protokołów TCP i QUIC
# Utworzono: 06.07.2021
# Autor: Anna Pawlus

cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
set(DIR_MVFST ${CMAKE_CURRENT_SOURCE_DIR}/mvfst/_build)
set(DIR_DEPS ${CMAKE_CURRENT_SOURCE_DIR}/mvfst/_build/deps)
set(DEPS_LIB ${DIR_DEPS}/lib/cmake)

execute_process(
	COMMAND ../build_helper.sh -s
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mvfst
	RESULT_VARIABLE result
)
if (result)
	message(ERROR " Could not install dependencies of mvfst, code: ${result}")
endif()

execute_process(
	COMMAND sed --in-place s/find_dependency/find_package/g ${DEPS_LIB}/fizz/fizz-config.cmake
	RESULT_VARIABLE result
)
if (result)
	message(ERROR " Could not edit fizz-config.cmake, code: ${result}")
endif()

include_directories(${DIR_MVFST})

set(fmt_DIR ${DEPS_LIB}/fmt)
set(folly_DIR ${DEPS_LIB}/folly)
set(Fizz_DIR ${DEPS_LIB}/fizz)

find_package(Fizz REQUIRED)
find_package(folly REQUIRED)
find_package(gflags REQUIRED)
add_subdirectory(mvfst)

add_executable(mvserver server.cpp)
target_link_libraries(mvserver PUBLIC
	mvfst_transport
	mvfst_server
	mvfst_client
	fizz::fizz
	mvfst_fizz_client
)

add_executable(mvclient client.cpp)
target_link_libraries(mvclient PUBLIC
	utils
	mvfst_transport
	mvfst_server
	mvfst_client
	fizz::fizz
	mvfst_fizz_client
)
