cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")

set(DIR_MVFST "mvfst/_build")
set(DIR_DEPS "mvfst/_build/deps")

list(APPEND CMAKE_PREFIX_PATH "${DIR_DEPS};${DIR_MVFST}")


include_directories(${DIR_MVFST} ${DIR_DEPS})

find_package(gflags REQUIRED)
find_package(mvfst REQUIRED)

add_executable(mvserver server.cpp)
target_link_libraries(mvserver PUBLIC
	mvfst::mvfst_transport
	mvfst::mvfst_server
	mvfst::mvfst_client
	fizz::fizz
	mvfst::mvfst_fizz_client
)

add_executable(mvclient client.cpp)
target_link_libraries(mvclient PUBLIC
	utils
	mvfst::mvfst_transport
	mvfst::mvfst_server
	mvfst::mvfst_client
	fizz::fizz
	mvfst::mvfst_fizz_client

)
