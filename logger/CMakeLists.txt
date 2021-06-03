add_executable(logger_test test.cpp)
target_link_libraries(logger_test gtest_main rt)

add_test(NAME logger_test COMMAND logger_test)