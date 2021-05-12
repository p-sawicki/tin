cmake_minimum_required(VERSION 3.10)

project(tin)

add_subdirectory(picoquic_impl)

add_executable(run_scenarios run_scenarios.cpp)